# C1-Slim / MP-D261 root-ADB admission helper.
# Original author and method: fwz233 (https://github.com/fwz233)
# Public-repository integration: C1 Slim Ports contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

from __future__ import annotations

import argparse
import datetime as dt
import ipaddress
import json
import os
import re
import signal
import socket
import ssl
import subprocess
import tempfile
import threading
import time
import uuid
from dataclasses import dataclass
from pathlib import Path
from urllib.parse import parse_qs, urlsplit

try:
    import pydivert
    from cryptography import x509
    from cryptography.hazmat.primitives import hashes, serialization
    from cryptography.hazmat.primitives.asymmetric import rsa
    from cryptography.x509.oid import NameOID
    from pydivert import Direction, Flag, Layer
    from pydivert.packet import Packet
except ModuleNotFoundError as error:
    raise SystemExit(
        f"Missing Python dependency {error.name!r}. Run: "
        "py -3.12 -m pip install cryptography pydivert==3.1.3"
    ) from error

HERE = Path(__file__).resolve().parent
LOG_PATH = HERE / "adb-admit.log"
DEFAULT_HOST = "api.mpen.com.cn"
DEFAULT_HOTSPOT_IP = "192.168.137.1"
DEFAULT_PORT = 443
MAX_REQUEST_BYTES = 64 * 1024
ADB_ROUTE = re.compile(r"/v1/pens/([^/]+)")
APPROVAL_BODY = json.dumps(
    {"errorCode": "200", "errorMsg": "", "data": {"success": True}},
    separators=(",", ":"),
).encode("utf-8")


@dataclass(frozen=True)
class Config:
    host: str
    origin_ip: str
    hotspot_ip: str
    interface_index: int
    device_ip: str
    port: int
    keep_running: bool


class Logger:
    def __init__(self, path: Path) -> None:
        self._path = path
        self._lock = threading.Lock()
        path.write_text("", encoding="utf-8")

    def write(self, message: str) -> None:
        line = f"{dt.datetime.now().astimezone().isoformat()} {message}"
        with self._lock:
            print(line, flush=True)
            with self._path.open("a", encoding="utf-8") as output:
                output.write(line + "\n")


def powershell(script: str) -> str:
    completed = subprocess.run(
        [
            "powershell.exe",
            "-NoProfile",
            "-NonInteractive",
            "-Command",
            "$ErrorActionPreference = 'Stop'; " + script,
        ],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    if completed.returncode != 0:
        detail = (completed.stderr or completed.stdout).strip()
        raise RuntimeError(f"PowerShell failed ({completed.returncode}): {detail}")
    return completed.stdout.strip()


def require_windows() -> None:
    if os.name != "nt":
        raise RuntimeError("this script only supports Windows 10/11")


def require_administrator() -> None:
    result = powershell(
        "([Security.Principal.WindowsPrincipal] "
        "[Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole("
        "[Security.Principal.WindowsBuiltInRole]::Administrator)"
    )
    if result.lower() != "true":
        raise RuntimeError("run this script from an Administrator PowerShell")


def validate_ip(value: str, label: str) -> str:
    try:
        return str(ipaddress.IPv4Address(value))
    except ipaddress.AddressValueError as error:
        raise ValueError(f"invalid {label}: {value!r}") from error


def discover_hotspot(requested_index: int | None) -> tuple[str, int]:
    index_filter = (
        f" | Where-Object {{ $_.InterfaceIndex -eq {requested_index} }}"
        if requested_index is not None
        else ""
    )
    output = powershell(
        "$adapters = @(Get-NetAdapter -IncludeHidden -ErrorAction SilentlyContinue) | "
        "Where-Object { $_.Status -eq 'Up' -and ("
        "$_.InterfaceDescription -like '*Wi-Fi Direct Virtual Adapter*' -or "
        "$_.Name -like '*Local Area Connection*') }"
        f"{index_filter}; "
        "$addresses = foreach ($adapter in $adapters) { "
        "Get-NetIPAddress -InterfaceIndex $adapter.ifIndex -AddressFamily IPv4 "
        "-ErrorAction SilentlyContinue | Where-Object { "
        "$_.AddressState -eq 'Preferred' -and $_.PrefixOrigin -ne 'WellKnown' "
        "} | ForEach-Object { '{0},{1}' -f $_.IPAddress, $_.InterfaceIndex } }; "
        "$addresses = @($addresses | Sort-Object -Unique); "
        "if ($addresses.Count -eq 0) { throw 'active mobile hotspot not found' }; "
        "if ($addresses.Count -gt 1) { throw ('multiple hotspot adapters found: ' + "
        "($addresses -join '; ') + '; pass --hotspot-ip or --interface-index') }; "
        "Write-Output $addresses[0]"
    )
    hotspot_ip, interface_index = output.splitlines()[-1].split(",", 1)
    return validate_ip(hotspot_ip, "discovered hotspot IP"), int(interface_index)


def discover_interface(hotspot_ip: str, requested_index: int | None) -> int:
    index_filter = (
        f" | Where-Object {{ $_.InterfaceIndex -eq {requested_index} }}"
        if requested_index is not None
        else ""
    )
    output = powershell(
        f"$address = @(Get-NetIPAddress -AddressFamily IPv4 -IPAddress '{hotspot_ip}' "
        f"-ErrorAction SilentlyContinue){index_filter} | "
        "Where-Object { $_.AddressState -eq 'Preferred' } | Select-Object -First 1; "
        "if ($null -eq $address) { throw 'active mobile hotspot address not found' }; "
        "Write-Output $address.InterfaceIndex"
    )
    return int(output.splitlines()[-1])


def discover_device_ip(
    interface_index: int,
    hotspot_ip: str,
    requested_ip: str | None,
    requested_mac: str | None,
) -> str:
    if requested_ip:
        device_ip = validate_ip(requested_ip, "device IP")
        output = powershell(
            f"@(Get-NetNeighbor -InterfaceIndex {interface_index} "
            f"-IPAddress '{device_ip}' -AddressFamily IPv4 "
            "-ErrorAction SilentlyContinue | Where-Object { "
            "$_.State -notin @('Unreachable', 'Incomplete') }).Count"
        )
        if int(output.splitlines()[-1]) == 0:
            raise RuntimeError(
                f"device {device_ip} is not present on hotspot interface "
                f"{interface_index}"
            )
        return device_ip

    mac_filter = ""
    if requested_mac:
        normalized_mac = requested_mac.replace(":", "-").upper()
        if re.fullmatch(r"(?:[0-9A-F]{2}-){5}[0-9A-F]{2}", normalized_mac) is None:
            raise ValueError(f"invalid device MAC: {requested_mac!r}")
        mac_filter = f" | Where-Object {{ $_.LinkLayerAddress -eq '{normalized_mac}' }}"

    prefix = hotspot_ip.rsplit(".", 1)[0] + ".*"
    output = powershell(
        f"@(Get-NetNeighbor -InterfaceIndex {interface_index} -AddressFamily IPv4 "
        "-ErrorAction SilentlyContinue) | "
        f"Where-Object {{ $_.IPAddress -like '{prefix}' -and "
        "$_.LinkLayerAddress -notlike '00-00-00-00-00-00' -and "
        "$_.LinkLayerAddress -notlike 'FF-FF-FF-FF-FF-FF' }}"
        f"{mac_filter} | ForEach-Object {{ $_.IPAddress }}"
    )
    candidates = sorted({line.strip() for line in output.splitlines() if line.strip()})
    if not candidates:
        raise RuntimeError(
            "no device found on the mobile hotspot; connect it first or pass --device-ip"
        )
    if len(candidates) > 1:
        raise RuntimeError(
            f"multiple hotspot clients found: {candidates}; pass --device-ip explicitly"
        )
    return validate_ip(candidates[0], "discovered device IP")


def resolve_origin(host: str, requested_ip: str | None) -> str:
    if requested_ip:
        return validate_ip(requested_ip, "origin IP")
    return validate_ip(socket.gethostbyname(host), "resolved origin IP")


class FirewallLease:
    def __init__(self, config: Config, logger: Logger) -> None:
        self._config = config
        self._logger = logger
        self._name = f"C1Slim-AdbAdmit-{uuid.uuid4()}"
        self._owned = False

    def acquire(self) -> None:
        powershell(
            f"New-NetFirewallRule -Name '{self._name}' -DisplayName '{self._name}' "
            "-Direction Inbound -Action Allow -Enabled True -Profile Any "
            f"-Protocol TCP -LocalAddress '{self._config.hotspot_ip}' "
            f"-LocalPort {self._config.port} -RemoteAddress '{self._config.device_ip}' "
            "| Out-Null"
        )
        self._owned = True
        self._logger.write(
            f"FIREWALL_ADDED rule={self._name} remote={self._config.device_ip}"
        )

    def release(self) -> None:
        if not self._owned:
            return
        try:
            powershell(
                f"Remove-NetFirewallRule -Name '{self._name}' "
                "-ErrorAction SilentlyContinue"
            )
            self._logger.write(f"FIREWALL_REMOVED rule={self._name}")
        except Exception as error:  # noqa: BLE001
            self._logger.write(f"FIREWALL_CLEANUP_FAILED error={error!r}")
        self._owned = False


def create_certificate(directory: Path, config: Config) -> tuple[Path, Path]:
    key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
    subject = issuer = x509.Name(
        [x509.NameAttribute(NameOID.COMMON_NAME, config.host)]
    )
    now = dt.datetime.now(dt.timezone.utc)
    certificate = (
        x509.CertificateBuilder()
        .subject_name(subject)
        .issuer_name(issuer)
        .public_key(key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(now - dt.timedelta(minutes=5))
        .not_valid_after(now + dt.timedelta(days=1))
        .add_extension(
            x509.SubjectAlternativeName(
                [
                    x509.DNSName(config.host),
                    x509.IPAddress(ipaddress.ip_address(config.origin_ip)),
                ]
            ),
            critical=False,
        )
        .sign(key, hashes.SHA256())
    )
    cert_path = directory / "certificate.pem"
    key_path = directory / "private-key.pem"
    cert_path.write_bytes(certificate.public_bytes(serialization.Encoding.PEM))
    key_path.write_bytes(
        key.private_bytes(
            serialization.Encoding.PEM,
            serialization.PrivateFormat.PKCS8,
            serialization.NoEncryption(),
        )
    )
    return cert_path, key_path


def build_tls_context(
    directory: Path,
    config: Config,
    logger: Logger,
) -> ssl.SSLContext:
    cert_path, key_path = create_certificate(directory, config)
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.minimum_version = ssl.TLSVersion.TLSv1_2
    context.load_cert_chain(cert_path, key_path)

    def record_sni(
        _socket: ssl.SSLObject,
        server_name: str | None,
        _context: ssl.SSLContext,
    ) -> None:
        logger.write(f"TLS_SNI server_name={server_name!r}")

    context.sni_callback = record_sni
    return context


def expected_request_size(request: bytes) -> int | None:
    marker = b"\r\n\r\n"
    header_end = request.find(marker)
    if header_end < 0:
        return None
    content_length = 0
    for line in request[:header_end].split(b"\r\n")[1:]:
        name, separator, value = line.partition(b":")
        if separator and name.strip().lower() == b"content-length":
            content_length = int(value.strip())
            break
    return header_end + len(marker) + content_length


def receive_request(connection: ssl.SSLSocket) -> bytes:
    request = bytearray()
    expected_size: int | None = None
    while expected_size is None or len(request) < expected_size:
        chunk = connection.recv(8192)
        if not chunk:
            break
        request.extend(chunk)
        if len(request) > MAX_REQUEST_BYTES:
            raise ValueError(f"request exceeds {MAX_REQUEST_BYTES} bytes")
        expected_size = expected_request_size(request)
    return bytes(request)


def parse_request(request: bytes) -> tuple[str, str, dict[str, list[str]]]:
    header_block = request.split(b"\r\n\r\n", 1)[0]
    lines = header_block.decode("iso-8859-1").split("\r\n")
    if not lines or len(lines[0].split()) != 3:
        raise ValueError("invalid HTTP request line")
    method, target, _version = lines[0].split()
    parsed = urlsplit(target)
    return method, parsed.path, parse_qs(parsed.query, keep_blank_values=True)


def is_adb_admit(method: str, path: str, query: dict[str, list[str]]) -> bool:
    if method != "GET" or ADB_ROUTE.fullmatch(path) is None:
        return False
    if set(query) - {"action", "random"} or query.get("action") != ["adbAdmit"]:
        return False
    random_values = query.get("random")
    return random_values is None or (
        len(random_values) == 1 and random_values[0].isdigit()
    )


def http_response(status: str, body: bytes) -> bytes:
    return (
        f"HTTP/1.1 {status}\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        f"Content-Length: {len(body)}\r\n"
        "Cache-Control: no-store\r\n"
        "Connection: close\r\n"
        "\r\n"
    ).encode("ascii") + body


def handle_connection(
    client: socket.socket,
    address: tuple[str, int],
    context: ssl.SSLContext,
    config: Config,
    logger: Logger,
    approved: threading.Event,
) -> None:
    peer = f"{address[0]}:{address[1]}"
    logger.write(f"TCP_ACCEPT peer={peer}")
    if address[0] != config.device_ip:
        logger.write(f"TCP_REJECT peer={peer} expected_ip={config.device_ip}")
        client.close()
        return

    try:
        with context.wrap_socket(client, server_side=True) as tls:
            logger.write(
                f"TLS_OK peer={peer} version={tls.version()} cipher={tls.cipher()}"
            )
            tls.settimeout(5)
            request = receive_request(tls)
            method, path, query = parse_request(request)
            safe_path = (
                "/v1/pens/<redacted>" if ADB_ROUTE.fullmatch(path) else "<unmatched>"
            )
            logger.write(
                f"HTTP_REQUEST peer={peer} method={method!r} path={safe_path!r} "
                f"query_keys={sorted(query)!r}"
            )
            if is_adb_admit(method, path, query):
                tls.sendall(http_response("200 OK", APPROVAL_BODY))
                logger.write(
                    f"APPROVED peer={peer} response_status='success'"
                )
                approved.set()
            else:
                body = b'{"error":"not found"}'
                tls.sendall(http_response("404 Not Found", body))
                logger.write(
                    f"REJECTED_ROUTE peer={peer} method={method!r} "
                    f"path={safe_path!r} query_keys={sorted(query)!r}"
                )
    except ssl.SSLError as error:
        logger.write(f"TLS_FAIL peer={peer} error={error!r}")
    except Exception as error:  # noqa: BLE001
        logger.write(f"CONNECTION_ERROR peer={peer} error={error!r}")
    finally:
        try:
            client.close()
        except OSError:
            pass


def reset_stale_connection(
    request: Packet,
    injector: pydivert.WinDivert,
    config: Config,
) -> int:
    reset = Packet(
        request.raw.tobytes(),
        interface=(config.interface_index, 0),
        direction=Direction.OUTBOUND,
        layer=Layer.NETWORK,
    )
    reset.src_addr = config.origin_ip
    reset.dst_addr = request.src_addr
    reset.src_port = config.port
    reset.dst_port = request.src_port
    if reset.tcp is None or request.tcp is None:
        return 0
    reset.payload = b""
    reset.tcp.seq_num = request.tcp.ack_num
    reset.tcp.ack_num = 0
    reset.tcp.control_bits = 0x004
    reset.tcp.window_size = 0
    return injector.send(reset)


def relay_responses(
    responses: pydivert.WinDivert,
    config: Config,
    logger: Logger,
) -> None:
    try:
        for response in responses:
            response.src_addr = config.origin_ip
            responses.send(response)
            logger.write(
                f"RESPONSE {config.origin_ip}:{config.port} -> "
                f"{response.dst_addr}:{response.dst_port} redirected"
            )
    except OSError:
        pass


def redirect_traffic(
    config: Config,
    logger: Logger,
    ready: threading.Event,
    stop: threading.Event,
    errors: list[BaseException],
    handles: list[pydivert.WinDivert],
) -> None:
    request_filter = (
        f"ip.SrcAddr == {config.device_ip} and "
        f"ip.DstAddr == {config.origin_ip} and tcp.DstPort == {config.port}"
    )
    response_filter = (
        f"outbound and ip.SrcAddr == {config.hotspot_ip} and "
        f"ip.DstAddr == {config.device_ip} and tcp.SrcPort == {config.port}"
    )
    try:
        with (
            pydivert.WinDivert(
                request_filter,
                layer=Layer.NETWORK_FORWARD,
                priority=100,
            ) as requests,
            pydivert.WinDivert(
                response_filter,
                layer=Layer.NETWORK,
                priority=100,
            ) as responses,
            pydivert.WinDivert(
                "false",
                layer=Layer.NETWORK,
                flags=Flag.SEND_ONLY,
            ) as injector,
        ):
            handles.extend((requests, responses, injector))
            logger.write(
                f"REDIRECT_READY device={config.device_ip} "
                f"origin={config.origin_ip}:{config.port} "
                f"local={config.hotspot_ip}:{config.port}"
            )
            ready.set()
            threading.Thread(
                target=relay_responses,
                args=(responses, config, logger),
                daemon=True,
            ).start()
            active_ports: set[int] = set()
            reset_ports: set[int] = set()
            for request in requests:
                if stop.is_set():
                    return
                if request.tcp is None or request.src_port is None:
                    continue
                source_port = request.src_port
                if request.tcp.syn and not request.tcp.ack:
                    active_ports.add(source_port)
                    reset_ports.discard(source_port)
                elif source_port not in active_ports:
                    if source_port not in reset_ports:
                        sent_bytes = reset_stale_connection(request, injector, config)
                        reset_ports.add(source_port)
                        logger.write(
                            f"RESET_STALE {config.origin_ip}:{config.port} -> "
                            f"{request.src_addr}:{source_port} bytes={sent_bytes}"
                        )
                    continue

                original_interface = request.interface
                request.dst_addr = config.hotspot_ip
                injected = Packet(
                    request.raw.tobytes(),
                    interface=(config.interface_index, 0),
                    direction=Direction.INBOUND,
                    layer=Layer.NETWORK,
                )
                sent_bytes = injector.send(injected)
                logger.write(
                    f"REQUEST {request.src_addr}:{source_port} -> "
                    f"{config.origin_ip}:{config.port} redirected "
                    f"forward_interface={original_interface} "
                    f"injected_interface={(config.interface_index, 0)} "
                    f"bytes={sent_bytes}"
                )
    except BaseException as error:  # noqa: BLE001
        if not stop.is_set():
            errors.append(error)
            logger.write(f"REDIRECT_FATAL error={error!r}")
            stop.set()
    finally:
        handles.clear()
        ready.set()


def ensure_clean_origin_address(origin_ip: str) -> None:
    output = powershell(
        f"@(Get-NetIPAddress -AddressFamily IPv4 -IPAddress '{origin_ip}' "
        "-ErrorAction SilentlyContinue).Count"
    )
    if int(output.splitlines()[-1]) != 0:
        raise RuntimeError(
            f"stale local address {origin_ip} exists; remove it before running"
        )


def ensure_port_available(hotspot_ip: str, port: int) -> None:
    output = powershell(
        f"@(Get-NetTCPConnection -LocalPort {port} -State Listen "
        "-ErrorAction SilentlyContinue | Where-Object { "
        f"$_.LocalAddress -eq '{hotspot_ip}' -or $_.LocalAddress -eq '0.0.0.0' "
        "}).Count"
    )
    if int(output.splitlines()[-1]) != 0:
        raise RuntimeError(f"TCP port {hotspot_ip}:{port} is already in use")


def run(config: Config, logger: Logger) -> None:
    stop = threading.Event()
    approved = threading.Event()
    redirect_ready = threading.Event()
    redirect_errors: list[BaseException] = []
    redirect_handles: list[pydivert.WinDivert] = []
    firewall = FirewallLease(config, logger)

    def request_stop(_signal: int, _frame: object) -> None:
        logger.write("STOP_REQUESTED")
        stop.set()

    signal.signal(signal.SIGINT, request_stop)
    signal.signal(signal.SIGTERM, request_stop)

    with tempfile.TemporaryDirectory(prefix="c1slim-adb-admit-") as temp:
        context = build_tls_context(Path(temp), config, logger)
        try:
            firewall.acquire()
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
                listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                listener.bind((config.hotspot_ip, config.port))
                listener.listen(8)
                listener.settimeout(0.5)
                logger.write(
                    f"HTTPS_READY address={config.hotspot_ip}:{config.port} "
                    f"device={config.device_ip}"
                )

                threading.Thread(
                    target=redirect_traffic,
                    args=(
                        config,
                        logger,
                        redirect_ready,
                        stop,
                        redirect_errors,
                        redirect_handles,
                    ),
                    daemon=True,
                ).start()
                if not redirect_ready.wait(5) or redirect_errors:
                    raise RuntimeError(
                        f"transparent redirect failed to start: {redirect_errors!r}"
                    )
                logger.write(
                    "READY open About-device and press keyboard Enter 10 times "
                    "within 5 seconds"
                )

                exit_deadline: float | None = None
                while not stop.is_set():
                    if redirect_errors:
                        raise RuntimeError(
                            f"transparent redirect failed: {redirect_errors!r}"
                        )
                    if approved.is_set() and not config.keep_running:
                        if exit_deadline is None:
                            exit_deadline = time.monotonic() + 2
                            logger.write("APPROVAL_SENT automatic cleanup in 2 seconds")
                        elif time.monotonic() >= exit_deadline:
                            break
                    try:
                        client, address = listener.accept()
                    except TimeoutError:
                        continue
                    threading.Thread(
                        target=handle_connection,
                        args=(client, address, context, config, logger, approved),
                        daemon=True,
                    ).start()
        finally:
            stop.set()
            for handle in tuple(redirect_handles):
                try:
                    handle.close()
                except OSError:
                    pass
            firewall.release()
            logger.write("STOPPED cleanup_complete=true hotspot_address_unchanged=true")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Capture and locally approve the C1-Slim adbAdmit request through a "
            "Windows mobile hotspot"
        )
    )
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--origin-ip", help="override the resolved API IPv4 address")
    parser.add_argument(
        "--hotspot-ip",
        help=f"override the detected hotspot gateway (usually {DEFAULT_HOTSPOT_IP})",
    )
    parser.add_argument("--interface-index", type=int)
    parser.add_argument("--device-ip")
    parser.add_argument(
        "--device-mac",
        help=(
            "device MAC shown by Windows Mobile Hotspot; omit when the C1-Slim "
            "is the hotspot's only client"
        ),
    )
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument(
        "--keep-running",
        action="store_true",
        help="do not stop automatically after one approved request",
    )
    return parser.parse_args()


def build_config(args: argparse.Namespace) -> Config:
    require_windows()
    require_administrator()
    if not 1 <= args.port <= 65535:
        raise ValueError(f"invalid TCP port: {args.port}")
    if args.hotspot_ip:
        hotspot_ip = validate_ip(args.hotspot_ip, "hotspot IP")
        interface_index = discover_interface(hotspot_ip, args.interface_index)
    else:
        hotspot_ip, interface_index = discover_hotspot(args.interface_index)
    device_ip = discover_device_ip(
        interface_index,
        hotspot_ip,
        args.device_ip,
        args.device_mac,
    )
    origin_ip = resolve_origin(args.host, args.origin_ip)
    ensure_clean_origin_address(origin_ip)
    ensure_port_available(hotspot_ip, args.port)
    return Config(
        host=args.host,
        origin_ip=origin_ip,
        hotspot_ip=hotspot_ip,
        interface_index=interface_index,
        device_ip=device_ip,
        port=args.port,
        keep_running=args.keep_running,
    )


def main() -> None:
    args = parse_args()
    logger = Logger(LOG_PATH)
    try:
        config = build_config(args)
        logger.write(
            f"START interface={config.interface_index} device={config.device_ip} "
            f"origin={config.origin_ip}:{config.port} log={LOG_PATH}"
        )
        logger.write(
            "PRIVACY request bodies, headers, identifiers, and query values are not logged"
        )
        run(config, logger)
    except Exception as error:  # noqa: BLE001
        logger.write(f"FATAL error={error!r}")
        raise


if __name__ == "__main__":
    main()
