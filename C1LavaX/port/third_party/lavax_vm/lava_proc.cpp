#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <cctype>
#include <cmath>
#include <queue>

#include "lava.h"
#include "lava_proc.h"

#ifndef DEBUG_PRINT
#define DEBUG_PRINT 0
#endif

#define TODO()      std::cerr << "PROC_TODO:" << __LINE__ << " " << __PRETTY_FUNCTION__ << std::endl
#define PROC_TODO() std::cerr << "PROC_TODO:" << __LINE__ << " " << __PRETTY_FUNCTION__ << std::endl

#if DEBUG_PRINT
#define DEBUG_WRAP_PRINT(v) std::cerr << v
#else
#define DEBUG_WRAP_PRINT(...)
#endif


void LavaProc::load(const std::vector<uint8_t> &source, uint32_t rambits, bool pen_input)
{
    this->source = source;
    this->rambits = rambits;
    this->pen_input = pen_input;

    ram.init(rambits);
    pc = 16;    // Skip file header
    parse_secret = 0;
    op_exec.clear();

    // Parse and cache all instructions
    uint32_t ofs = pc;
    while (ofs < source.size() && source[ofs] != 0 && source[ofs] != 0xff) {
        uint32_t s = parse(ofs);
        ofs += s;
    }

#if DEBUG_PRINT
    std::cerr << "OP code:" << std::endl;

    for (auto const &op: op_exec) {
        std::cerr << std::showbase << std::internal << std::setfill('0');
        std::cerr << std::hex << std::setw(10) << op.first << ": ";
        std::cerr << op_info[source[op.first]].name << std::endl;
    }
#endif
}

uint32_t LavaProc::parse(uint32_t ofs)
{
    uint8_t opcode = source[ofs];
    if (op_info.find(opcode) == op_info.cend()) {
        std::ostringstream error;
        error << "Unsupported opcode 0x" << std::hex << static_cast<unsigned int>(opcode)
              << " at file offset 0x" << ofs;
        throw std::runtime_error(error.str());
    }
    op_info_t info = op_info[opcode];

    uint32_t size = info.len & OpParamBaseMask;
    if (info.len & OpParamAddr)
        size += rambits <= 16 ? 2 : 3;
    if (info.len & OpParamStr) {
        uint32_t i = ofs + 1;
        while ((source.at(i) ^ parse_secret) != '\0')
            i++;
        size += i - ofs;
    }
    if (info.len & OpParamPreset) {
        uint16_t len = source.at(ofs + size - 2) | (source.at(ofs + size - 1) << 8);
        size += len;
    }
    std::vector<uint8_t> data(source.begin() + ofs + 1, source.begin() + ofs + size);
    if (info.len & OpParamStr)
        for (uint8_t &byte : data)
            byte ^= parse_secret;
    if (info.func) {
        op_exec[ofs] = op_t{std::bind(info.func, this, data)};
    } else {
        op_exec[ofs] = op_t{std::bind(&LavaProc::lava_wrap_extended, this, opcode, data)};
    }
    if (opcode == 0x43 && !data.empty())
        parse_secret = data[0];
    return size;
}

void LavaProc::reset()
{
    ram.init(rambits);
    pc = 16;
    cb_func.func = nullptr;
    refresh_req = true;
    console_small = false;
    console_x = 0;
    console_y = 0;
}

void LavaProc::run()
{
    if (debug_break) {
        if (cb->debugContinue() == -1)
            return;
        debug_break = false;
    }

    if (cb_func.func) {
        int32_t ret = cb_func.func();
        if (ret < 0)
            return;
        if (cb_func.stack) {
            ram.pop();
            ram.push(ret);
        }
        cb_func.func = nullptr;
    }

    const auto op_it = op_exec.find(pc);
    if (op_it == op_exec.end()) {
        std::ostringstream error;
        error << "Program counter reached a non-instruction offset 0x" << std::hex << pc;
        throw std::runtime_error(error.str());
    }
    auto const &op = op_it->second;

#if DEBUG_PRINT
    static bool print = true;
    if (print) {
        std::cerr << "OP execution:" << std::endl;
        print = false;
    }

    std::cerr << std::noshowbase << std::internal << std::setfill('0');
    std::cerr << std::hex << std::setw(8) << pc << " ";
    std::cerr << std::setw(4) << ram.getStack() << "-";
    std::cerr << std::setw(4) << ram.getFuncStackStart() << "-";
    std::cerr << std::setw(4) << ram.getFuncStackEnd() << ": ";
    std::cerr << std::showbase;

#endif

    cb_func.pc = pc;
    cb_func.sp = ram.getStack();
    op.func();

#if 0
    bool trigger = ram.readU8(0x587c) == 0x90 && ram.readU8(0x587d) == 0x0c;
    static bool trigger_p = false;
    if (trigger && !trigger_p)
        debug_break = true;
    trigger_p = trigger;
#endif

    // Only request display refresh when host operation is needed
    if (refresh_req && disp->refreshRequest())
        cb->refresh(disp->getFramebuffer());
    refresh_req = false;
}

void LavaProc::saveState(std::ostream &ss)
{
    auto const pushU32 = [&](uint32_t v)
    {
        ss.put(v >>  0);
        ss.put(v >>  8);
        ss.put(v >> 16);
        ss.put(v >> 24);
    };

    // Common configurations
    pushU32(rambits);
    pushU32(pen_input);

    // System state
    if (cb_func.func) {
        // Save state before waiting for callback
        pushU32(cb_func.pc);
        pushU32(cb_func.sp);
    } else {
        // Current state is ready for save state
        pushU32(pc);
        pushU32(ram.getStack());
    }
    // Instructions requiring callback do no affect these, so should be safe
    pushU32(ram.getFuncStackStart());
    pushU32(ram.getFuncStackEnd());
    pushU32(ram.getStringStack());
    pushU32(flagv);
    pushU32(seed);

    ss.write(reinterpret_cast<const char *>(ram.data().data()), ram.data().size());

    // Save file descriptors
    ss.put(file_map.size());
    for (const auto &f: file_map) {
        uint8_t fd = f.first;
        ss.put(fd);
        auto const &path = f.second.path;
        pushU32(path.size());
        ss.write(path.data(), path.size());
        auto const &mode = f.second.mode;
        pushU32(mode.size());
        ss.write(mode.data(), mode.size());
        int32_t offset = cb->ftell(fd);
        pushU32(offset);
        if (offset < 0)
            std::cerr << "Failed to save file state: " << f.second.path << std::endl;
        //std::cerr << __func__ << "(\"" << path << "\", \"" << mode << "\", " << offset << ")" << std::endl;
    }
}

void LavaProc::restoreState(std::istream &ss)
{
    auto const popU32 = [&]() -> uint32_t
    {
        uint32_t v = 0;
        v |= ss.get() <<  0;
        v |= ss.get() <<  8;
        v |= ss.get() << 16;
        v |= ss.get() << 24;
        return v;
    };

    rambits = popU32();
    pen_input = popU32();
    ram.init(rambits);

    cb_func.func = nullptr;
    pc = popU32();
    ram.setStack(popU32());
    ram.setFuncStackStart(popU32());
    ram.setFuncStackEnd(popU32());
    ram.setStringStack(popU32());
    flagv = popU32();
    seed = popU32();

    ss.read(reinterpret_cast<char *>(ram.data().data()), ram.data().size());

    // Restore file descriptors
    while (!file_map.empty())
        lava_op_fclose(file_map.begin()->first);
    uint8_t nfd = ss.get();
    for (int i = 0; i < nfd; i++) {
        uint8_t fd = ss.get();
        std::string path(popU32(), '\0');
        ss.read(path.data(), path.size());
        std::string mode(popU32(), '\0');
        ss.read(mode.data(), mode.size());
        int32_t offset = popU32();
        if (offset >= 0 && cb->frestore(fd, path, mode, offset))
            file_map[fd] = {path, mode};
        else
            std::cerr << "Failed to restore file state: " << path << std::endl;
    }

    refresh_req = true;
}

#include "lava_op_code.h"
#include "lava_proc_printf.h"

void LavaProc::lava_wrap_pass(const std::vector<uint8_t> &data)
{
    // LavaX 3.5 compiler annotation: consume its one-byte payload and do
    // nothing, matching the original VM's c_pass implementation.
    pc += 1 + data.size();
}

void LavaProc::lava_wrap_void(const std::vector<uint8_t> &data)
{
    pc += 1 + data.size();
}

void LavaProc::lava_wrap_letx(const std::vector<uint8_t> &data)
{
    const uint32_t value = ram.pop();
    uint32_t address = ram.pop();
    uint8_t type = data.at(0);
    if (type & 0x80)
        address += ram.getFuncStackStart();
    type &= 0x7f;
    if (type == 1)
        ram.writeU8(address, static_cast<uint8_t>(value));
    else if (type == 2)
        ram.writeI16(address, static_cast<int16_t>(value));
    else
        ram.writeU32(address, value);
    ram.push(value);
    pc += 1 + data.size();
}

void LavaProc::lava_wrap_idx(const std::vector<uint8_t> &data)
{
    uint32_t address = ram.pop();
    const uint8_t descriptor = data.at(0);
    if (descriptor & 0x80)
        address += ram.getFuncStackStart();

    const uint8_t width = descriptor & 0x1f;
    uint32_t value;
    if (width == 1)
        value = ram.readU8(address);
    else if (width == 2)
        value = static_cast<int32_t>(ram.readI16(address));
    else
        value = ram.readU32(address);

    const uint8_t operation = (descriptor >> 5) & 0x03;
    const uint32_t result = operation < 2
        ? (operation == 0 ? value + 1 : value - 1)
        : value;
    const uint32_t stored = operation == 0 || operation == 2 ? value + 1 : value - 1;

    if (width == 1)
        ram.writeU8(address, static_cast<uint8_t>(stored));
    else if (width == 2)
        ram.writeI16(address, static_cast<int16_t>(stored));
    else
        ram.writeU32(address, stored);
    ram.push(result);
    pc += 1 + data.size();
}

void LavaProc::lava_wrap_push_ax(const std::vector<uint8_t> &data)
{
    uint32_t base = static_cast<uint32_t>(data.at(0)) |
                    (static_cast<uint32_t>(data.at(1)) << 8);
    if (data.size() == 3)
        base |= static_cast<uint32_t>(data.at(2)) << 16;
    const uint32_t offset = ram.pop();
    ram.push((base + offset) & ram.getAddrMask());
    pc += 1 + data.size();
}

void LavaProc::lava_wrap_tolower(const std::vector<uint8_t> &data)
{
    const uint32_t value = ram.pop();
    ram.push(static_cast<uint32_t>(std::tolower(static_cast<unsigned char>(value))));
    pc += 1 + data.size();
}

void LavaProc::lava_wrap_toupper(const std::vector<uint8_t> &data)
{
    const uint32_t value = ram.pop();
    ram.push(static_cast<uint32_t>(std::toupper(static_cast<unsigned char>(value))));
    pc += 1 + data.size();
}

void LavaProc::lava_wrap_putchar(const std::vector<uint8_t> &data)
{
    const uint8_t value = static_cast<uint8_t>(ram.pop());
    pc += 1 + data.size();
    consoleWrite(std::vector<uint8_t>{value, 0});
}

void LavaProc::consoleWrite(const std::vector<uint8_t> &text)
{
    const uint16_t glyph_width = console_small ? 6 : 8;
    const uint16_t row_height = console_small ? 13 : 16;
    const uint16_t columns = console_small
        ? static_cast<uint16_t>(((disp->getFramebufferWidth() - 2) / 6) & ~1)
        : static_cast<uint16_t>(disp->getFramebufferWidth() / 8);
    const uint16_t rows = console_small
        ? static_cast<uint16_t>((disp->getFramebufferHeight() - 1) / 13)
        : static_cast<uint16_t>(disp->getFramebufferHeight() / 16);
    const uint16_t x_offset = console_small
        ? static_cast<uint16_t>((disp->getFramebufferWidth() - columns * 6) / 2) : 0;
    const uint16_t y_offset = console_small
        ? static_cast<uint16_t>((disp->getFramebufferHeight() - (rows * 13 - 1)) / 2) : 0;
    const uint8_t cfg = console_small ? 0 : 0x80;

    for (size_t i = 0; i < text.size() && text[i] != 0; ++i) {
        const uint8_t value = text[i];
        if (value == '\r') {
            console_x = 0;
            continue;
        }
        if (value == '\n') {
            console_x = 0;
            ++console_y;
            continue;
        }
        if (value == '\b') {
            if (console_x)
                --console_x;
            continue;
        }
        const bool double_width = value >= 0x80 && i + 1 < text.size() && text[i + 1] != 0;
        const uint16_t cells = double_width ? 2 : 1;
        if (console_x + cells > columns) {
            console_x = 0;
            ++console_y;
        }
        if (console_y >= rows) {
            disp->clearWorking();
            console_x = 0;
            console_y = 0;
        }
        std::vector<uint8_t> glyph{value};
        if (double_width)
            glyph.push_back(text[++i]);
        glyph.push_back(0);
        disp->drawText(glyph, x_offset + console_x * glyph_width,
                       y_offset + console_y * row_height, cfg);
        console_x += cells;
    }
    disp->framebufferFlush();
    refresh_req = true;
}

void LavaProc::lava_wrap_makedir(const std::vector<uint8_t> &data)
{
    const uint32_t path_address = ram.pop();
    const std::string path = to_string(ram.readStringData(path_address));
    ram.push(cb->makeDir(path));
    pc += 1 + data.size();
}

void LavaProc::lava_wrap_getfilenum(const std::vector<uint8_t> &data)
{
    const uint32_t path_address = ram.pop();
    const std::string path = to_string(ram.readStringData(path_address));
    const auto entries = cb->listDir(path);
#if DEBUG_PRINT
    std::cerr << "getfilenum(path=\"" << path << "\", count=" << entries.size() << ")\n";
#endif
    ram.push(static_cast<uint32_t>(entries.size()));
    pc += 1 + data.size();
}

void LavaProc::lava_wrap_findfile(const std::vector<uint8_t> &data)
{
    const uint32_t name_address = ram.pop() & ram.getAddrMask();
    const uint32_t count = ram.pop();
    const uint32_t index = ram.pop();
    auto entries = cb->listDir(".");
    entries.insert(entries.begin(), "..");
#if DEBUG_PRINT
    std::cerr << "findfile(index=" << index << ", count=" << count
              << ", available=" << entries.size() << ")\n";
#endif

    uint32_t written = 0;
    for (; written < count && index + written < entries.size(); ++written) {
        const std::string &name = entries[index + written];
        const uint32_t slot = name_address + written * 16;
        const size_t length = std::min<size_t>(name.size(), 15);
        for (size_t i = 0; i < length; ++i)
            ram.writeU8(slot + i, static_cast<uint8_t>(name[i]));
        ram.writeU8(slot + length, 0);
    }
    ram.push(written);
    pc += 1 + data.size();
}

void LavaProc::lava_wrap_chdir(const std::vector<uint8_t> &data)
{
    const uint32_t path_address = ram.pop();
    const std::string path = to_string(ram.readStringData(path_address));
    ram.push(cb->changeDir(path));
    pc += 1 + data.size();
}

void LavaProc::lava_wrap_exec(const std::vector<uint8_t> &data)
{
    const uint32_t settings = ram.pop();
    const uint32_t command_address = ram.pop();
    const uint32_t path_address = ram.pop();
    const std::string command = to_string(ram.readStringData(command_address));
    const std::string path = to_string(ram.readStringData(path_address));
    ram.push(static_cast<uint32_t>(cb->exec(path, command, settings)));
    pc += 1 + data.size();
    refresh_req = true;
}

void LavaProc::lava_wrap_setpalette(const std::vector<uint8_t> &data)
{
    (void)ram.pop(); // palette data address
    const uint32_t count = ram.pop();
    (void)ram.pop(); // first palette index
    ram.push(count);
    pc += 1 + data.size();
}

void LavaProc::lava_wrap_extended(uint8_t opcode, const std::vector<uint8_t> &data)
{
    pc += 1 + data.size();
    const auto float_from_bits = [](uint32_t bits) {
        float value;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    };
    const auto bits_from_float = [](float value) {
        uint32_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        return bits;
    };
    const auto push_predicate = [&](bool value) {
        ram.push(value ? Lava::True : Lava::False);
    };
    const auto address_param = [&]() {
        uint32_t address = static_cast<uint32_t>(data.at(0)) |
                           (static_cast<uint32_t>(data.at(1)) << 8);
        if (data.size() >= 3)
            address |= static_cast<uint32_t>(data.at(2)) << 16;
        return address;
    };

    switch (opcode) {
    case 0x15: {
        const uint32_t address = address_param() + ram.pop() + ram.getFuncStackStart();
        ram.push(ram.getAddrVariant(address, 2));
        break;
    }
    case 0x1a: case 0x1b: case 0x42:
        ram.push(0);
        break;
    case 0x25:
        ram.push(~ram.pop());
        break;
    case 0x43: case 0x44: case 0x73: case 0x74:
        break;
    case 0x52: {
        const uint32_t address = ram.pop() & ram.getAddrMask();
        ram.push(static_cast<int32_t>(ram.readI16(address)));
        break;
    }
    case 0x53: {
        const uint32_t address = ram.pop() & ram.getAddrMask();
        ram.push(ram.readU32(address));
        break;
    }
    case 0x54:
        ram.push(bits_from_float(static_cast<float>(static_cast<int32_t>(ram.pop()))));
        break;
    case 0x55:
        ram.push(static_cast<int32_t>(float_from_bits(ram.pop())));
        break;
    case 0x56: case 0x57: case 0x58: case 0x59: case 0x5a: case 0x5b:
    case 0x5c: case 0x5d: case 0x5e: case 0x5f: case 0x60: case 0x61: {
        const uint32_t right_bits = ram.pop();
        const uint32_t left_bits = ram.pop();
        const float left = opcode == 0x58 || opcode == 0x5b || opcode == 0x5e || opcode == 0x61
            ? static_cast<float>(static_cast<int32_t>(left_bits)) : float_from_bits(left_bits);
        const float right = opcode == 0x57 || opcode == 0x5a || opcode == 0x5d || opcode == 0x60
            ? static_cast<float>(static_cast<int32_t>(right_bits)) : float_from_bits(right_bits);
        float value = 0;
        if (opcode <= 0x58) value = left + right;
        else if (opcode <= 0x5b) value = left - right;
        else if (opcode <= 0x5e) value = left * right;
        else value = right == 0 ? 0 : left / right;
        ram.push(bits_from_float(value));
        break;
    }
    case 0x62:
        ram.push(bits_from_float(-float_from_bits(ram.pop())));
        break;
    case 0x63: case 0x64: case 0x65: case 0x66: case 0x67: case 0x68: {
        const float right = float_from_bits(ram.pop());
        const float left = float_from_bits(ram.pop());
        bool value = false;
        if (opcode == 0x63) value = left < right;
        else if (opcode == 0x64) value = left > right;
        else if (opcode == 0x65) value = left == right;
        else if (opcode == 0x66) value = left != right;
        else if (opcode == 0x67) value = left <= right;
        else value = left >= right;
        push_predicate(value);
        break;
    }
    case 0x69:
        ram.push(ram.pop() & 0x7fffffffU);
        break;
    case 0x6a:
        ram.push(ram.getAddrVariant(ram.pop(), 2));
        break;
    case 0x6b:
        ram.push(ram.getAddrVariant(ram.pop(), 4));
        break;
    case 0x6c:
        ram.push(ram.pop() & 0xff);
        break;
    case 0x6d: {
        uint32_t value = ram.pop() & 0xffff;
        if (value & 0x8000)
            value |= 0xffff0000U;
        ram.push(value);
        break;
    }
    case 0x86:
        (void)ram.pop();
        refresh_req = true;
        break;
    case 0x8f: {
        const int32_t value = static_cast<int32_t>(ram.pop());
        ram.push(value < 0 ? static_cast<uint32_t>(-static_cast<int64_t>(value)) : value);
        break;
    }
    case 0x95: {
        const int16_t y = static_cast<int16_t>(ram.pop());
        const int16_t x = static_cast<int16_t>(ram.pop());
        ram.push(disp->getPoint(x, y, 0x40));
        break;
    }
    case 0x97: {
        const uint8_t cfg = static_cast<uint8_t>(ram.pop() & 3) | 0x40;
        const bool filled = ram.pop() != 0;
        const int16_t y1 = static_cast<int16_t>(ram.pop());
        const int16_t x1 = static_cast<int16_t>(ram.pop());
        const int16_t y0 = static_cast<int16_t>(ram.pop());
        const int16_t x0 = static_cast<int16_t>(ram.pop());
        if (filled)
            disp->drawBlock(x0, x1, y0, y1, cfg);
        else
            disp->drawRectangle(x0, x1, y0, y1, cfg);
        refresh_req = true;
        break;
    }
    case 0x99: {
        const uint8_t cfg = static_cast<uint8_t>(ram.pop()) ^ 0x40;
        const bool filled = ram.pop() != 0;
        const uint16_t ry = static_cast<uint16_t>(ram.pop());
        const uint16_t rx = static_cast<uint16_t>(ram.pop());
        const int16_t y = static_cast<int16_t>(ram.pop());
        const int16_t x = static_cast<int16_t>(ram.pop());
        disp->drawEllipse(x, y, rx, ry, filled, cfg);
        break;
    }
    case 0x9a:
        break;
    case 0x9b: case 0x9d: case 0x9e: case 0x9f: case 0xa0:
    case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: {
        const unsigned char value = static_cast<unsigned char>(ram.pop());
        bool result = false;
        if (opcode == 0x9b) result = std::isalnum(value) != 0;
        else if (opcode == 0x9d) result = std::iscntrl(value) != 0;
        else if (opcode == 0x9e) result = std::isdigit(value) != 0;
        else if (opcode == 0x9f) result = std::isgraph(value) != 0;
        else if (opcode == 0xa0) result = std::islower(value) != 0;
        else if (opcode == 0xa1) result = std::isprint(value) != 0;
        else if (opcode == 0xa2) result = std::ispunct(value) != 0;
        else if (opcode == 0xa3) result = std::isspace(value) != 0;
        else if (opcode == 0xa4) result = std::isupper(value) != 0;
        else result = std::isxdigit(value) != 0;
        push_predicate(result);
        break;
    }
    case 0xb3:
        ram.push(static_cast<uint32_t>(cb->ftell(static_cast<uint8_t>(ram.pop()))));
        break;
    case 0xb4:
        push_predicate(cb->feof(static_cast<uint8_t>(ram.pop())));
        break;
    case 0xb6: {
        const auto bytes = cb->fread(static_cast<uint8_t>(ram.pop()), 1);
        ram.push(bytes.empty() ? 0xffffffffU : bytes[0]);
        break;
    }
    case 0xb7: {
        const uint8_t fd = static_cast<uint8_t>(ram.pop());
        const uint8_t value = static_cast<uint8_t>(ram.pop());
        const int32_t written = cb->fwrite(fd, std::vector<uint8_t>{value});
        ram.push(written == 1 ? value : 0xffffffffU);
        break;
    }
    case 0xbd: {
        const uint32_t length = ram.pop() & ram.getAddrMask();
        const uint32_t source_address = ram.pop() & ram.getAddrMask();
        const uint32_t destination = ram.pop() & ram.getAddrMask();
        std::memmove(ram.data().data() + destination,
                     ram.data().data() + source_address, length);
        break;
    }
    case 0xc1: {
        const uint32_t destination = ram.pop() & ram.getAddrMask();
        ram.writeU8(destination, 0);
        ram.push(Lava::False);
        break;
    }
    case 0xc3:
        (void)ram.pop();
        break;
    case 0xc4:
        (void)ram.pop();
        cb_func.func = std::bind(&LavaCallback::getchar, cb);
        cb_func.stack = true;
        ram.push(0);
        refresh_req = true;
        break;
    case 0xc8: case 0xc9: {
        constexpr double pi = 3.14159265358979323846;
        const int32_t degrees = static_cast<int32_t>(ram.pop()) % 360;
        const double radians = degrees * pi / 180.0;
        ram.push(static_cast<int32_t>(std::lround((opcode == 0xc8 ? std::sin(radians) : std::cos(radians)) * 1024.0)));
        break;
    }
    case 0xca:
        (void)ram.pop(); (void)ram.pop(); (void)ram.pop();
        break;
    case 0xce:
        (void)ram.pop(); (void)ram.pop();
        break;
    case 0xcf:
        disp->fade(static_cast<uint8_t>(ram.pop()));
        refresh_req = true;
        break;
    case 0xd3: {
        const uint32_t operation = ram.pop();
        uint32_t result = 0;
        if (operation == 1 || operation == 6 || operation == 8)
            (void)ram.pop();
        else if (operation == 9 || operation == 10 || operation == 11 || operation == 15)
            { (void)ram.pop(); (void)ram.pop(); }
        else if (operation == 12 || operation == 20)
            { (void)ram.pop(); (void)ram.pop(); (void)ram.pop(); }
        else if (operation == 29)
            for (int i = 0; i < 5; ++i) (void)ram.pop();
        else if (operation == 30 || operation == 32 || operation == 33)
            { (void)ram.pop(); (void)ram.pop(); }
        if (operation == 31)
            result = static_cast<uint32_t>(cb->getMs() * 256 / 1000);
        ram.push(result);
        break;
    }
    case 0xd4: {
        const uint32_t operation = ram.pop();
        if (operation == 0) {
            ram.push(0x100);
            break;
        }
        const uint32_t bits = ram.pop();
        float value = float_from_bits(bits);
        if (operation == 7) value = std::sin(value);
        else if (operation == 8) value = std::cos(value);
        else if (operation == 9) value = std::tan(value);
        else if (operation == 10) value = std::asin(value);
        else if (operation == 11) value = std::acos(value);
        else if (operation == 12) value = std::atan(value);
        else if (operation == 13) value = std::sqrt(value);
        else if (operation == 14) value = std::exp(value);
        else if (operation == 15) value = std::log(value);
        else if (operation == 19) value = std::fabs(value);
        else value = 0;
        ram.push(bits_from_float(value));
        break;
    }
    case 0xd6: {
        const uint32_t destination = ram.pop() & ram.getAddrMask();
        std::string command = cb->getCommandLine();
        if (command.size() > 255)
            command.resize(255);
        std::vector<uint8_t> bytes(command.begin(), command.end());
        bytes.push_back(0);
        ram.writeData(destination, bytes);
        break;
    }
    default:
        throw std::runtime_error("unimplemented extended opcode " + std::to_string(opcode));
    }
}

uint32_t LavaProc::lava_op_push_u8(uint8_t dp0)
{
    return dp0;
}

uint32_t LavaProc::lava_op_push_i16(int16_t dp0)
{
    return (int32_t)dp0;
}

uint32_t LavaProc::lava_op_push_i32(int32_t dp0)
{
    return dp0;
}

uint32_t LavaProc::lava_op_pushv_u8(uint32_t dp0)
{
    return ram.readU8(dp0);
}

uint32_t LavaProc::lava_op_pushv_i16(uint32_t dp0)
{
    return (int32_t)ram.readI16(dp0);
}

uint32_t LavaProc::lava_op_pushv_i32(uint32_t dp0)
{
    return ram.readI32(dp0);
}

uint32_t LavaProc::lava_op_pushg_u8(uint32_t dp0, uint32_t ds0)
{
    return ram.readU8(dp0 + ds0);
}

uint32_t LavaProc::lava_op_pushg_i16(uint32_t dp0, uint32_t ds0)
{
    return (int32_t)ram.readI16(dp0 + ds0);
}

uint32_t LavaProc::lava_op_pushg_i32(uint32_t dp0, uint32_t ds0)
{
    return ram.readI32(dp0 + ds0);
}

uint32_t LavaProc::lava_op_pusha_u8(uint32_t dp0, uint32_t ds0)
{
    uint32_t a = dp0 + ds0;
    return ram.getAddrVariant(a, 1);
}

uint32_t LavaProc::lava_op_pusha_i16(uint32_t dp0, uint32_t ds0)
{
    uint32_t a = dp0 + ds0;
    return ram.getAddrVariant(a, 2);
}

uint32_t LavaProc::lava_op_pusha_i32(uint32_t dp0, uint32_t ds0)
{
    uint32_t a = dp0 + ds0;
    return ram.getAddrVariant(a, 0);
}

uint32_t LavaProc::lava_op_push_str(const std::vector<uint8_t> &dp0)
{
    return ram.pushString(dp0);
}

uint32_t LavaProc::lava_op_pushlv_u8(uint32_t dp0)
{
    uint32_t addr = dp0 + ram.getFuncStackStart();
    return ram.readU8(addr);
}

uint32_t LavaProc::lava_op_pushlv_i16(uint32_t dp0)
{
    uint32_t addr = dp0 + ram.getFuncStackStart();
    return (int32_t)ram.readI16(addr);
}

uint32_t LavaProc::lava_op_pushlv_i32(uint32_t dp0)
{
    uint32_t addr = dp0 + ram.getFuncStackStart();
    return ram.readI32(addr);
}

uint32_t LavaProc::lava_op_pushlg_char(uint32_t dp0, uint32_t ds0)
{
    uint32_t addr = dp0 + ds0 + ram.getFuncStackStart();
    return ram.readU8(addr);
}

uint32_t LavaProc::lava_op_pushlg_int(uint32_t dp0, uint32_t ds0)
{
    uint32_t addr = dp0 + ds0 + ram.getFuncStackStart();
    return (int32_t)ram.readI16(addr);
}

uint32_t LavaProc::lava_op_pushlg_long(uint32_t dp0, uint32_t ds0)
{
    uint32_t addr = dp0 + ds0 + ram.getFuncStackStart();
    return ram.readI32(addr);
}

uint32_t LavaProc::lava_op_pushla_u8(uint32_t dp0, uint32_t ds0)
{
    uint32_t a = dp0 + ds0 + ram.getFuncStackStart();
    return ram.getAddrVariant(a, 1);
}

uint32_t LavaProc::lava_op_pushla_i32(uint32_t dp0, uint32_t ds0)
{
    uint32_t a = dp0 + ds0 + ram.getFuncStackStart();
    return ram.getAddrVariant(a, 0);
}

uint32_t LavaProc::lava_op_pushl_i32(uint32_t dp0)
{
    uint32_t a = dp0 + ram.getFuncStackStart();
    return a & ram.getAddrMask();
}

uint32_t LavaProc::lava_op_neg(uint32_t ds0)
{
    return -(int32_t)ds0;
}

uint32_t LavaProc::lava_op_pre_inc(uint32_t ds0)
{
    int32_t addr = ds0;
    int32_t v = ram.readVariant(addr) + 1;
    ram.writeVariant(addr, v);
    return v;
}

uint32_t LavaProc::lava_op_pre_dec(uint32_t ds0)
{
    int32_t addr = ds0;
    int32_t v = ram.readVariant(addr) - 1;
    ram.writeVariant(addr, v);
    return v;
}

uint32_t LavaProc::lava_op_post_inc(uint32_t ds0)
{
    int32_t addr = ds0;
    int32_t v = ram.readVariant(addr);
    ram.writeVariant(addr, v + 1);
    return v;
}

uint32_t LavaProc::lava_op_post_dec(uint32_t ds0)
{
    int32_t addr = ds0;
    int32_t v = ram.readVariant(addr);
    ram.writeVariant(addr, v - 1);
    return v;
}

uint32_t LavaProc::lava_op_add(uint32_t ds0, uint32_t ds1)
{
    return (int32_t)ds1 + (int32_t)ds0;
}

uint32_t LavaProc::lava_op_sub(uint32_t ds0, uint32_t ds1)
{
    return (int32_t)ds1 - (int32_t)ds0;
}

uint32_t LavaProc::lava_op_and(uint32_t ds0, uint32_t ds1)
{
    return ds1 & ds0;
}

uint32_t LavaProc::lava_op_or(uint32_t ds0, uint32_t ds1)
{
    return ds1 | ds0;
}

uint32_t LavaProc::lava_op_xor(uint32_t ds0, uint32_t ds1)
{
    return ds1 ^ ds0;
}

uint32_t LavaProc::lava_op_land(uint32_t ds0, uint32_t ds1)
{
    return (ds0 && ds1) ? Lava::True : Lava::False;
}

uint32_t LavaProc::lava_op_lor(uint32_t ds0, uint32_t ds1)
{
    return (ds0 || ds1) ? Lava::True : Lava::False;
}

uint32_t LavaProc::lava_op_lnot(uint32_t ds0)
{
    return ds0 ? Lava::False : Lava::True;
}

uint32_t LavaProc::lava_op_mul(uint32_t ds0, uint32_t ds1)
{
    return (int32_t)ds1 * (int32_t)ds0;
}

uint32_t LavaProc::lava_op_div(uint32_t ds0, uint32_t ds1)
{
    if (static_cast<int32_t>(ds0) == 0)
        return 0;
    return (int32_t)ds1 / (int32_t)ds0;
}

uint32_t LavaProc::lava_op_mod(uint32_t ds0, uint32_t ds1)
{
    int32_t a3 = ds0;
    int32_t a1 = ds1;
    if (a3 == 0)
        return 0;
    return a1 % a3;
}

uint32_t LavaProc::lava_op_lshift(uint32_t ds0, uint32_t ds1)
{
    int32_t a3 = ds0;
    int32_t a1 = ds1;

    if (a3 < 0)
        return 0;
    return a1 << a3;
}

uint32_t LavaProc::lava_op_rshift(uint32_t ds0, uint32_t ds1)
{
    int32_t a3 = ds0;
    int32_t a1 = ds1;

    if (a3 < 0)
        return 0;
    return (uint32_t)a1 >> a3;
}

uint32_t LavaProc::lava_op_equ(uint32_t ds0, uint32_t ds1)
{
    return ds1 == ds0 ? Lava::True : Lava::False;
}

uint32_t LavaProc::lava_op_neq(uint32_t ds0, uint32_t ds1)
{
    return ds1 != ds0 ? Lava::True : Lava::False;
}

uint32_t LavaProc::lava_op_le(uint32_t ds0, uint32_t ds1)
{
    return ((int32_t)ds1 <= (int32_t)ds0) ? Lava::True : Lava::False;
}

uint32_t LavaProc::lava_op_ge(uint32_t ds0, uint32_t ds1)
{
    return ((int32_t)ds1 >= (int32_t)ds0) ? Lava::True : Lava::False;
}

uint32_t LavaProc::lava_op_great(uint32_t ds0, uint32_t ds1)
{
    return ((int32_t)ds1 > (int32_t)ds0) ? Lava::True : Lava::False;
}

uint32_t LavaProc::lava_op_less(uint32_t ds0, uint32_t ds1)
{
    return ((int32_t)ds1 < (int32_t)ds0) ? Lava::True : Lava::False;
}

uint32_t LavaProc::lava_op_let(uint32_t ds0, uint32_t ds1)
{
    int32_t v = ds0;
    int32_t a = ds1;
    ram.writeVariant(a, v);
    return v;
}

uint32_t LavaProc::lava_op_ptr(uint32_t ds0)
{
    return ram.readU8(ds0);
}

uint32_t LavaProc::lava_op_cptr(uint32_t ds0)
{
    return (ds0 & ram.getAddrMask()) | (0x01 << (8 * ram.getAddrBytes()));
}

void LavaProc::lava_op_pop(uint32_t ds0)
{
    flagv = ds0;
}

void LavaProc::lava_op_jmpe(uint32_t dp0)
{
    if (!flagv)
        pc = dp0;
}

void LavaProc::lava_op_jmpn(uint32_t dp0)
{
    if (flagv)
        pc = dp0;
}

void LavaProc::lava_op_jmp(uint32_t dp0)
{
    pc = dp0;
}

void LavaProc::lava_op_set_sp(uint32_t dp0)
{
    ram.setFuncStackEnd(dp0);
}

void LavaProc::lava_op_call(uint32_t dp0)
{
    // Function call, first step, PC jump

    // Write PC address of next instruction to next stack frame
    ram.writeU24(ram.getFuncStackEnd(), pc);
    lava_op_jmp(dp0);
}

void LavaProc::lava_op_push_frame(uint32_t dp0, uint8_t dp1)
{
    // Function call, second step, update stack frame

    // PC address already written by previous call(), skip
    uint32_t ofs = ram.getFuncStackEnd() + 3;

    // Write start of previous stack frame
    ram.writeAddr(ofs, ram.getFuncStackStart());
    ofs += ram.getAddrBytes();

    // Special offset for 32-bit mode?
    if (rambits >= 32)
        ofs += 1;

    // Write stack frame variables
    uint32_t nwords = dp1;
    while (nwords--) {
        uint32_t v = ram.pop();
        ram.writeU32(ofs + nwords * 4, v);
    }

    // Update stack frame start & end pointers to current frame
    uint32_t frame_size = dp0;
    ram.setFuncStackStart(ram.getFuncStackEnd());
    ram.setFuncStackEnd(ram.getFuncStackEnd() + frame_size);
}

void LavaProc::lava_op_pop_frame()
{
    // Function call return

    // Read current stack frame
    uint32_t sp = ram.getFuncStackStart();

    // Read and jump to return PC address
    pc = ram.readU24(sp);

    // Read previous stack frame start
    sp = ram.readAddr(sp + 3);

    // Update stack frame start & end pointers to previous frame
    ram.setFuncStackEnd(ram.getFuncStackStart());
    ram.setFuncStackStart(sp);
}

void LavaProc::lava_op_quit()
{
    lava_op_exit(0);
}

void LavaProc::lava_op_preset(uint32_t dp0, const std::vector<uint8_t> &dp2)
{
    ram.writeData(dp0, dp2);
}

uint32_t LavaProc::lava_op_qadd(int16_t dp0, uint32_t ds0)
{
    return (int32_t)ds0 + (int32_t)dp0;
}

uint32_t LavaProc::lava_op_qsub(int16_t dp0, uint32_t ds0)
{
    return (int32_t)ds0 - (int32_t)dp0;
}

uint32_t LavaProc::lava_op_qmul(int16_t dp0, uint32_t ds0)
{
    return (int32_t)ds0 * (int32_t)dp0;
}

uint32_t LavaProc::lava_op_qdiv(int16_t dp0, uint32_t ds0)
{
    if (dp0 == 0)
        return 0;
    return (int32_t)ds0 / (int32_t)dp0;
}

uint32_t LavaProc::lava_op_qmod(int16_t dp0, uint32_t ds0)
{
    if (dp0 == 0)
        return 0;
    return (int32_t)ds0 % dp0;
}

uint32_t LavaProc::lava_op_qlshift(int16_t dp0, uint32_t ds0)
{
    uint32_t v = ds0;
    int32_t shift = dp0;
#if 0
    return v << dp0;
#else
    if (shift >= 0)
        v <<= shift;
    else
        v >>= -shift;
    return v;
#endif
}

uint32_t LavaProc::lava_op_qrshift(int16_t dp0, uint32_t ds0)
{
    uint32_t v = ds0;
    int32_t shift = dp0;
#if 0
    return v >> dp0;
#else
    if (shift >= 0)
        v >>= shift;
    else
        v <<= -shift;
    return v;
#endif
}

uint32_t LavaProc::lava_op_qequ(int16_t dp0, uint32_t ds0)
{
    return (int32_t)ds0 == (int32_t)dp0 ? Lava::True : Lava::False;
}

uint32_t LavaProc::lava_op_qneq(int16_t dp0, uint32_t ds0)
{
    return (int32_t)ds0 != (int32_t)dp0 ? Lava::True : Lava::False;
}

uint32_t LavaProc::lava_op_qgreat(int16_t dp0, uint32_t ds0)
{
    return (int32_t)ds0 > (int32_t)dp0 ? Lava::True : Lava::False;
}

uint32_t LavaProc::lava_op_qless(int16_t dp0, uint32_t ds0)
{
    return (int32_t)ds0 < (int32_t)dp0 ? Lava::True : Lava::False;
}

uint32_t LavaProc::lava_op_qge(int16_t dp0, uint32_t ds0)
{
    return (int32_t)ds0 >= (int32_t)dp0 ? Lava::True : Lava::False;
}

uint32_t LavaProc::lava_op_qle(int16_t dp0, uint32_t ds0)
{
    return (int32_t)ds0 <= (int32_t)dp0 ? Lava::True : Lava::False;
}

uint32_t LavaProc::lava_op_getchar()
{
    cb_func.func = std::bind(&LavaCallback::getchar, cb);
    cb_func.stack = true;
    refresh_req = true;
    return 0;
}

void LavaProc::lava_op_setscreen(uint32_t ds0)
{
    console_small = ds0 != 0;
    console_x = 0;
    console_y = 0;
    disp->clearWorking();
}

void LavaProc::lava_op_delay(uint32_t ds0)
{
    cb_func.func = std::bind(&LavaCallback::delayMs, cb, ds0);
    cb_func.stack = false;
    refresh_req = true;
}

void LavaProc::lava_op_writeblock(uint32_t ds0, uint32_t ds1, uint32_t ds2, uint32_t ds3, uint32_t ds4, uint32_t ds5)
{
    uint32_t addr = ds0;
    uint8_t cfg = ds1;
    uint16_t h = ds2;
    uint16_t w = ds3;
    int16_t y = ds4;
    int16_t x = ds5;

    uint32_t size = 0;
    switch (disp->getMode()) {
    case LavaDisp::GraphicMono:
        size = ((w + 7) / 8) * h;
        break;
    case LavaDisp::Graphic16:
        size = ((w + 1) / 2) * h;
        break;
    case LavaDisp::Graphic256:
        size = w * h;
        break;
    }

    auto const &data = ram.readData(addr, size);
    disp->drawBlock(x, y, w, h, cfg, data);
}

void LavaProc::lava_op_fbflush()
{
    disp->framebufferFlush();
#if LAVA_REFRESH_AT_FLUSH
    refresh_req = true;
#endif
}

void LavaProc::lava_op_textout(uint32_t ds0, uint32_t ds1, uint32_t ds2, uint32_t ds3)
{
    uint8_t cfg = ds0;
    uint32_t addr = ds1;
    uint16_t x = ds3;
    uint16_t y = ds2;
    auto const &str = ram.readStringData(addr);
    //std::cerr << __func__ << ": " << addr << ", " << str.size() << std::endl;
    disp->drawText(str, x, y, cfg);
}

void LavaProc::lava_op_block(uint32_t ds0, uint32_t ds1, uint32_t ds2, uint32_t ds3, uint32_t ds4)
{
    uint8_t cfg = ds0;
    int16_t y1 = ds1;
    int16_t x1 = ds2;
    int16_t y0 = ds3;
    int16_t x0 = ds4;
    disp->drawBlock(x0, x1, y0, y1, cfg);
}

void LavaProc::lava_op_rectangle(uint32_t ds0, uint32_t ds1, uint32_t ds2, uint32_t ds3, uint32_t ds4)
{
    uint8_t cfg = ds0;
    int16_t y1 = ds1;
    int16_t x1 = ds2;
    int16_t y0 = ds3;
    int16_t x0 = ds4;
    disp->drawRectangle(x0, x1, y0, y1, cfg);
}

void LavaProc::lava_op_exit(uint32_t ds0)
{
    refresh_req = true;
    cb->exit(ds0);
}

void LavaProc::lava_op_clearscreen()
{
    disp->clearWorking();
}

uint32_t LavaProc::lava_op_rand()
{
    int32_t v = seed * 0x15a4e35 + 1;
    seed = v;
    return (v >> 16) & 0x7fff;
}

void LavaProc::lava_op_srand(uint32_t ds0)
{
    seed = ds0;
}

void LavaProc::lava_op_locate(uint32_t ds0, uint32_t ds1)
{
    console_x = static_cast<uint16_t>(ds0);
    console_y = static_cast<uint16_t>(ds1);
}

uint32_t LavaProc::lava_op_inkey()
{
    refresh_req = true;
    return cb->inKey();
}

void LavaProc::lava_op_point(uint32_t ds0, uint32_t ds1, uint32_t ds2)
{
    disp->drawPoint(static_cast<int16_t>(ds2), static_cast<int16_t>(ds1),
                    static_cast<uint8_t>(ds0) ^ 0x40);
}

void LavaProc::lava_op_line(uint32_t ds0, uint32_t ds1, uint32_t ds2, uint32_t ds3, uint32_t ds4)
{
    disp->drawLine(static_cast<int16_t>(ds4), static_cast<int16_t>(ds3),
                   static_cast<int16_t>(ds2), static_cast<int16_t>(ds1),
                   static_cast<uint8_t>(ds0) ^ 0x40);
}

void LavaProc::lava_op_circle(uint32_t ds0, uint32_t ds1, uint32_t ds2, uint32_t ds3, uint32_t ds4)
{
    disp->drawEllipse(static_cast<int16_t>(ds4), static_cast<int16_t>(ds3),
                      static_cast<uint16_t>(ds2), static_cast<uint16_t>(ds2),
                      ds1 != 0, static_cast<uint8_t>(ds0) ^ 0x40);
}

uint32_t LavaProc::lava_op_isalpha(uint32_t ds0)
{
    char c = ds0;
    if (c >= 'a' && c <= 'z')
        return Lava::True;
    if (c >= 'A' && c <= 'Z')
        return Lava::True;
    return Lava::False;
}

void LavaProc::lava_op_strcat(uint32_t ds0, uint32_t ds1)
{
    uint8_t *dst = ram.data().data() + (ds1 & ram.getAddrMask());
    uint8_t *src = ram.data().data() + (ds0 & ram.getAddrMask());

    while (*dst != 0)
        dst++;

    do
        *dst++ = *src;
    while (*src++ != 0);
}

uint32_t LavaProc::lava_op_strchr(uint32_t ds0, uint32_t ds1)
{
    const uint8_t *base = ram.data().data();
    const uint8_t *pstr = base + (ds1 & ram.getAddrMask());
    uint8_t c = ds0;

    const uint8_t *s = pstr;
    while (*s != 0 && *s != c)
        s++;
    if (*s == c)
        return static_cast<uint32_t>(s - base);
    return 0;
}

uint32_t LavaProc::lava_op_strcmp(uint32_t ds0, uint32_t ds1)
{
    const char *s1 = reinterpret_cast<const char *>(ram.data().data()) + (ds1 & ram.getAddrMask());
    const char *s2 = reinterpret_cast<const char *>(ram.data().data()) + (ds0 & ram.getAddrMask());
    for (; *s1 == *s2; s1++, s2++)
        if (*s1 == 0)
            return 0;
    if (*s1 < *s2)
        return -1;
    return 1;
}

uint32_t LavaProc::lava_op_strstr(uint32_t ds0, uint32_t ds1)
{
    const char *base = reinterpret_cast<const char *>(ram.data().data());
    const char *haystack = base + (ds1 & ram.getAddrMask());
    const char *needle   = base + (ds0 & ram.getAddrMask());

    const char *p = strstr(haystack, needle);
    if (p == nullptr)
        return 0;
    return static_cast<uint32_t>(p - base);
}

void LavaProc::lava_op_strcpy(uint32_t ds0, uint32_t ds1)
{
    uint8_t *dst = ram.data().data() + (ds1 & ram.getAddrMask());
    const uint8_t *src = ram.data().data() + (ds0 & ram.getAddrMask());
    do
        *dst++ = *src;
    while (*src++ != 0);
}

uint32_t LavaProc::lava_op_strlen(uint32_t ds0)
{
    uint32_t addr = ds0;
    return ram.strlen(addr);
}

void LavaProc::lava_op_memset(uint32_t ds0, uint32_t ds1, uint32_t ds2)
{
    uint32_t len = ds0 & ram.getAddrMask();
    uint8_t v = ds1;
    uint32_t addr = ds2 & ram.getAddrMask();
    std::fill(ram.data().begin() + addr, ram.data().begin() + addr + len, v);
}

void LavaProc::lava_op_memcpy(uint32_t ds0, uint32_t ds1, uint32_t ds2)
{
    uint32_t len = ds0 & ram.getAddrMask();
    uint32_t src = ds1 & ram.getAddrMask();
    uint32_t dst = ds2 & ram.getAddrMask();
    std::copy(ram.data().begin() + src, ram.data().begin() + src + len,
              ram.data().begin() + dst);
}

uint32_t LavaProc::lava_op_fopen(uint32_t ds0, uint32_t ds1)
{
    auto const &path = to_string(ram.readStringData(ds1));
    auto const &mode = to_string(ram.readStringData(ds0));
    int fd = cb->fopen(path, mode);
    if (fd != 0)
        file_map[fd] = {path, mode};
    return fd;
}

void LavaProc::lava_op_fclose(uint32_t ds0)
{
    int fd = ds0;
    file_map.erase(fd);
    cb->fclose(fd);
}

uint32_t LavaProc::lava_op_fread(uint32_t ds0, uint32_t ds1, uint32_t ds2, uint32_t ds3)
{
    uint8_t fd = ds0;
    uint32_t size = ds1 * ds2;
    uint32_t addr = ds3;

#if 0
    uint32_t offset = cb->ftell(fd);
    std::cerr << " => " << file_map[fd].path << ", " << offset;
#endif

    auto const &data = cb->fread(fd, size);
    ram.writeData(addr, data);
    return data.size();
}

uint32_t LavaProc::lava_op_fwrite(uint32_t ds0, uint32_t ds1, uint32_t ds2, uint32_t ds3)
{
    uint8_t fd = ds0;
    uint32_t size = ds1 * ds2;
    uint32_t addr = ds3;

    auto const &data = ram.readData(addr, size);
    return cb->fwrite(fd, data);
}

uint32_t LavaProc::lava_op_fseek(uint32_t ds0, uint32_t ds1, uint32_t ds2)
{
    LavaCallback::fseek_mode_t mode = (LavaCallback::fseek_mode_t)ds0;
    int32_t ofs = ds1;
    uint8_t fd = ds2;
    return cb->fseek(fd, ofs, mode);
}

void LavaProc::lava_op_rewind(uint32_t ds0)
{
    uint8_t fd = ds0;
    cb->fseek(fd, 0, LavaCallback::SeekSet);
}

uint32_t LavaProc::lava_op_deletefile(uint32_t ds0)
{
    uint32_t str = ds0 & ram.getAddrMask();
    std::string fpath(reinterpret_cast<const char *>(ram.data().data()) + str);
    return cb->remove(fpath);
}

uint32_t LavaProc::lava_op_crc16(uint32_t ds0, uint32_t ds1)
{
    static const uint8_t crc1[256]={
        0x0,0x21,0x42,0x63,0x84,0xa5,0xc6,0xe7,
        0x8,0x29,0x4a,0x6b,0x8c,0xad,0xce,0xef,
        0x31,0x10,0x73,0x52,0xb5,0x94,0xf7,0xd6,
        0x39,0x18,0x7b,0x5a,0xbd,0x9c,0xff,0xde,
        0x62,0x43,0x20,0x1,0xe6,0xc7,0xa4,0x85,
        0x6a,0x4b,0x28,0x9,0xee,0xcf,0xac,0x8d,
        0x53,0x72,0x11,0x30,0xd7,0xf6,0x95,0xb4,
        0x5b,0x7a,0x19,0x38,0xdf,0xfe,0x9d,0xbc,
        0xc4,0xe5,0x86,0xa7,0x40,0x61,0x2,0x23,
        0xcc,0xed,0x8e,0xaf,0x48,0x69,0xa,0x2b,
        0xf5,0xd4,0xb7,0x96,0x71,0x50,0x33,0x12,
        0xfd,0xdc,0xbf,0x9e,0x79,0x58,0x3b,0x1a,
        0xa6,0x87,0xe4,0xc5,0x22,0x3,0x60,0x41,
        0xae,0x8f,0xec,0xcd,0x2a,0xb,0x68,0x49,
        0x97,0xb6,0xd5,0xf4,0x13,0x32,0x51,0x70,
        0x9f,0xbe,0xdd,0xfc,0x1b,0x3a,0x59,0x78,
        0x88,0xa9,0xca,0xeb,0xc,0x2d,0x4e,0x6f,
        0x80,0xa1,0xc2,0xe3,0x4,0x25,0x46,0x67,
        0xb9,0x98,0xfb,0xda,0x3d,0x1c,0x7f,0x5e,
        0xb1,0x90,0xf3,0xd2,0x35,0x14,0x77,0x56,
        0xea,0xcb,0xa8,0x89,0x6e,0x4f,0x2c,0xd,
        0xe2,0xc3,0xa0,0x81,0x66,0x47,0x24,0x5,
        0xdb,0xfa,0x99,0xb8,0x5f,0x7e,0x1d,0x3c,
        0xd3,0xf2,0x91,0xb0,0x57,0x76,0x15,0x34,
        0x4c,0x6d,0xe,0x2f,0xc8,0xe9,0x8a,0xab,
        0x44,0x65,0x6,0x27,0xc0,0xe1,0x82,0xa3,
        0x7d,0x5c,0x3f,0x1e,0xf9,0xd8,0xbb,0x9a,
        0x75,0x54,0x37,0x16,0xf1,0xd0,0xb3,0x92,
        0x2e,0xf,0x6c,0x4d,0xaa,0x8b,0xe8,0xc9,
        0x26,0x7,0x64,0x45,0xa2,0x83,0xe0,0xc1,
        0x1f,0x3e,0x5d,0x7c,0x9b,0xba,0xd9,0xf8,
        0x17,0x36,0x55,0x74,0x93,0xb2,0xd1,0xf0
    };
    static const uint8_t crc2[256]={
        0x0,0x10,0x20,0x30,0x40,0x50,0x60,0x70,
        0x81,0x91,0xa1,0xb1,0xc1,0xd1,0xe1,0xf1,
        0x12,0x2,0x32,0x22,0x52,0x42,0x72,0x62,
        0x93,0x83,0xb3,0xa3,0xd3,0xc3,0xf3,0xe3,
        0x24,0x34,0x4,0x14,0x64,0x74,0x44,0x54,
        0xa5,0xb5,0x85,0x95,0xe5,0xf5,0xc5,0xd5,
        0x36,0x26,0x16,0x6,0x76,0x66,0x56,0x46,
        0xb7,0xa7,0x97,0x87,0xf7,0xe7,0xd7,0xc7,
        0x48,0x58,0x68,0x78,0x8,0x18,0x28,0x38,
        0xc9,0xd9,0xe9,0xf9,0x89,0x99,0xa9,0xb9,
        0x5a,0x4a,0x7a,0x6a,0x1a,0xa,0x3a,0x2a,
        0xdb,0xcb,0xfb,0xeb,0x9b,0x8b,0xbb,0xab,
        0x6c,0x7c,0x4c,0x5c,0x2c,0x3c,0xc,0x1c,
        0xed,0xfd,0xcd,0xdd,0xad,0xbd,0x8d,0x9d,
        0x7e,0x6e,0x5e,0x4e,0x3e,0x2e,0x1e,0xe,
        0xff,0xef,0xdf,0xcf,0xbf,0xaf,0x9f,0x8f,
        0x91,0x81,0xb1,0xa1,0xd1,0xc1,0xf1,0xe1,
        0x10,0x0,0x30,0x20,0x50,0x40,0x70,0x60,
        0x83,0x93,0xa3,0xb3,0xc3,0xd3,0xe3,0xf3,
        0x2,0x12,0x22,0x32,0x42,0x52,0x62,0x72,
        0xb5,0xa5,0x95,0x85,0xf5,0xe5,0xd5,0xc5,
        0x34,0x24,0x14,0x4,0x74,0x64,0x54,0x44,
        0xa7,0xb7,0x87,0x97,0xe7,0xf7,0xc7,0xd7,
        0x26,0x36,0x6,0x16,0x66,0x76,0x46,0x56,
        0xd9,0xc9,0xf9,0xe9,0x99,0x89,0xb9,0xa9,
        0x58,0x48,0x78,0x68,0x18,0x8,0x38,0x28,
        0xcb,0xdb,0xeb,0xfb,0x8b,0x9b,0xab,0xbb,
        0x4a,0x5a,0x6a,0x7a,0xa,0x1a,0x2a,0x3a,
        0xfd,0xed,0xdd,0xcd,0xbd,0xad,0x9d,0x8d,
        0x7c,0x6c,0x5c,0x4c,0x3c,0x2c,0x1c,0xc,
        0xef,0xff,0xcf,0xdf,0xaf,0xbf,0x8f,0x9f,
        0x6e,0x7e,0x4e,0x5e,0x2e,0x3e,0xe,0x1e
    };

    uint32_t src = ds1 & ram.getAddrMask();
    uint32_t len = ds0 & ram.getAddrMask();

    uint8_t c1, c2, x;
    c1 = 0;
    c2 = 0;
    while (len--) {
        x = c2 ^ ram.readU8(src++);
        c2 = crc2[x] ^ c1;
        c1 = crc1[x];
    }
    return c1 + (c2 << 8);
}

void LavaProc::lava_op_encrypt(uint32_t ds0, uint32_t ds1, uint32_t ds2)
{
    uint32_t str2 = ds0 & ram.getAddrMask();
    uint32_t len  = ds1 & ram.getAddrMask();
    uint32_t str1 = ds2 & ram.getAddrMask();

    uint32_t i = 0;
    while (len--) {
        uint8_t c = ram.readU8(str2 + i);
        i++;
        if (c == 0) {
            c = ram.readU8(str2);
            i = 1;
        }
        ram.writeU8(str1, ram.readU8(str1) ^ c);
        str1++;
    }
}

void LavaProc::lava_op_gettime(uint32_t ds0)
{
    uint8_t *p = ram.data().data() + (ds0 & ram.getAddrMask());
    const auto &time = cb->getTime();
    p[0] = time.year;
    p[1] = time.year >> 8;
    p[2] = time.month;
    p[3] = time.day;
    p[4] = time.hour;
    p[5] = time.minute;
    p[6] = time.second;
    p[7] = time.dayOfWeek;
}

void LavaProc::lava_op_xdraw(uint32_t ds0)
{
    disp->xdraw(ds0);
}

uint32_t LavaProc::lava_op_gettick()
{
    int32_t ms = cb->getMs();
    int32_t tick = (ms % 1000) * 256 / 1000;
    //std::cerr << __func__ << ": " << ms << ", " << tick << std::endl;
    return tick;
}

uint32_t LavaProc::lava_op_checkkey(uint32_t ds0)
{
    refresh_req = true;
    return cb->checkKey(ds0);
}

void LavaProc::lava_op_releasekey(uint32_t ds0)
{
    refresh_req = true;
    cb->releaseKey(ds0);
}

void LavaProc::lava_op_getblock(uint32_t ds0, uint32_t ds1, uint32_t ds2, uint32_t ds3, uint32_t ds4, uint32_t ds5)
{
    int32_t dst = ds0;
    int32_t cfg = ds1;
    uint16_t h = ds2;
    uint16_t w = ds3;
    uint16_t y = ds4;
    uint16_t x = ds5;

    ram.writeData(dst, disp->getBlock(x, y, w, h, cfg));
}

uint32_t LavaProc::lava_op_setgraphmode(uint32_t ds0)
{
    LavaDisp::mode_t mode = disp->getMode();
    disp->setMode((LavaDisp::mode_t)ds0);
    return (uint32_t)mode;
}

void LavaProc::lava_op_setbgcolor(uint32_t ds0)
{
    disp->setBackgroundColour(ds0);
}

void LavaProc::lava_op_setfgcolor(uint32_t ds0)
{
    disp->setForegroundColour(ds0);
}
