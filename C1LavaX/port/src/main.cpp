#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <linux/input.h>
#include <map>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "lava.h"

namespace {

constexpr const char *kVersion = "0.8.1";
constexpr const char *kDefaultRoot = "/storage/c1lavax/os";
constexpr const char *kDefaultFont = "/storage/c1lavax/LVM.bin";
constexpr const char *kDisplayDevice = "/dev/epaper_lcd";
constexpr int kPanelWidth = 296;
constexpr int kPanelHeight = 152;
constexpr int kPanelBytes = kPanelWidth * (kPanelHeight / 8);
constexpr uint64_t kMinimumPanelFrameMs = 80;
constexpr uint64_t kStableFrameMs = 140;

volatile std::sig_atomic_t interrupted = 0;

void signal_handler(int)
{
    interrupted = 1;
}

std::vector<uint8_t> read_file(const std::string &path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("cannot open " + path + ": " + std::strerror(errno));
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(input),
                                std::istreambuf_iterator<char>());
}

uint64_t monotonic_ms()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

class C1Platform final : public LavaCallback {
public:
    struct RunResult {
        bool host_exit = false;
        bool program_exit = false;
        int32_t exit_code = 0;
        std::string requested_program;
        std::string command_line;
    };

    C1Platform(std::string root, std::vector<uint8_t> font, bool stable_frames,
               bool mota_key_fix)
        : root_(trim_trailing_slash(std::move(root))), font_(std::move(font)),
          stable_frames_(stable_frames), mota_key_fix_(mota_key_fix)
    {
        frame_.fill(0);
        cached_.fill(0xff);
        open_inputs();
    }

    ~C1Platform() override
    {
        for (int fd : input_fds_)
            if (fd >= 0)
                ::close(fd);
        for (auto &file : files_)
            if (file.second)
                std::fclose(file.second);
    }

    int run(std::string initial_program)
    {
        const std::string shell = "/System/ShellX.sys";
        std::string program = std::move(initial_program);
        bool running_shell = same_virtual_path(program, shell);
        const bool direct_launch = !running_shell;
        std::string shell_return_dir = "/";
        if (!running_shell) {
            const std::string normalized = normalize_virtual(program);
            const size_t slash = normalized.find_last_of('/');
            current_dir_ = slash == 0 ? "/" : normalized.substr(0, slash);
        }

        for (;;) {
            RunResult result = run_one(program);
            if (result.host_exit || interrupted) {
                if (!interrupted)
                    wait_for_key_release();
                return 0;
            }
            if (!result.requested_program.empty()) {
                if (running_shell)
                    shell_return_dir = current_dir_;
                program = result.requested_program;
                command_line_ = result.command_line;
                running_shell = false;
                continue;
            }
            if (!running_shell) {
                // A program explicitly supplied by the stock-launcher bridge
                // is a single-app session.  Its exit must return to the stock
                // launcher, not fall through to the emulated LavaX shell.
                if (direct_launch) {
                    wait_for_key_release();
                    return result.exit_code;
                }
                current_dir_ = shell_return_dir;
                command_line_.clear();
                program = shell;
                running_shell = true;
                continue;
            }
            wait_for_key_release();
            return result.exit_code;
        }
    }

    void refresh(const uint8_t *framebuffer) override
    {
        if (!vm_)
            return;
        render(framebuffer, vm_->getFramebufferWidth(), vm_->getFramebufferHeight(),
               vm_->getGraphicMode());
    }

    void exit(uint32_t code) override
    {
        result_.program_exit = true;
        result_.exit_code = static_cast<int32_t>(code);
    }

    int32_t getchar() override
    {
        // A blocking key read is a natural scene boundary: title cards,
        // dialogue pages and menus are complete at this point.  Publishing
        // here avoids showing every intermediate glyph on an e-paper panel.
        flush_pending(true);
        pump_input(10);
        if (result_.host_exit || interrupted)
            return -1;
        if (key_queue_.empty())
            return -1;
        const uint8_t key = key_queue_.front();
        key_queue_.erase(key_queue_.begin());
        return key;
    }

    int32_t checkKey(uint8_t key) override
    {
        pump_input(0);
        return pressed_[key] ? Lava::True : Lava::False;
    }

    int32_t inKey() override
    {
        pump_input(0);
        return last_key_;
    }

    void releaseKey(uint8_t key) override
    {
        pressed_[key] = false;
        if (last_key_ == key)
            last_key_ = 0;
    }

    int32_t debugContinue() override { return 0; }

    uint8_t fopen(std::string path, std::string mode) override
    {
        const std::string full = resolve(path);
        if (full.empty())
            return 0;

        std::string clean_mode;
        for (char c : mode) {
            if (c == 'r' || c == 'w' || c == 'a' || c == '+')
                clean_mode.push_back(c);
        }
        if (clean_mode.empty())
            return 0;
        clean_mode.push_back('b');

        for (unsigned int fd = 0x80; fd <= 0x82; ++fd) {
            if (files_.count(fd))
                continue;
            FILE *file = std::fopen(full.c_str(), clean_mode.c_str());
            if (!file)
                return 0;
            files_[fd] = file;
            return static_cast<uint8_t>(fd);
        }
        return 0;
    }

    void fclose(uint8_t fd) override
    {
        auto found = files_.find(fd);
        if (found == files_.end())
            return;
        std::fclose(found->second);
        files_.erase(found);
    }

    std::vector<uint8_t> fread(uint8_t fd, uint32_t size) override
    {
        auto found = files_.find(fd);
        if (found == files_.end())
            return {};
        std::vector<uint8_t> data(size);
        data.resize(std::fread(data.data(), 1, size, found->second));
        return data;
    }

    int32_t fwrite(uint8_t fd, const std::vector<uint8_t> &data) override
    {
        auto found = files_.find(fd);
        if (found == files_.end())
            return 0;
        return static_cast<int32_t>(std::fwrite(data.data(), 1, data.size(), found->second));
    }

    int32_t fseek(uint8_t fd, int32_t offset, fseek_mode_t mode) override
    {
        auto found = files_.find(fd);
        if (found == files_.end())
            return -1;
        int whence = mode == SeekCur ? SEEK_CUR : mode == SeekEnd ? SEEK_END : SEEK_SET;
        if (std::fseek(found->second, offset, whence) != 0)
            return -1;
        return static_cast<int32_t>(std::ftell(found->second));
    }

    int32_t ftell(uint8_t fd) override
    {
        auto found = files_.find(fd);
        return found == files_.end() ? -1 : static_cast<int32_t>(std::ftell(found->second));
    }

    bool feof(uint8_t fd) override
    {
        auto found = files_.find(fd);
        if (found == files_.end())
            return true;
        const long position = std::ftell(found->second);
        if (position < 0 || std::fseek(found->second, 0, SEEK_END) != 0)
            return true;
        const long end = std::ftell(found->second);
        (void)std::fseek(found->second, position, SEEK_SET);
        return position >= end;
    }

    bool frestore(uint8_t fd, std::string path, std::string mode, int32_t offset) override
    {
        if (files_.count(fd))
            return false;
        const std::string full = resolve(path);
        FILE *file = std::fopen(full.c_str(), mode.c_str());
        if (!file)
            return false;
        if (std::fseek(file, offset, SEEK_SET) != 0) {
            std::fclose(file);
            return false;
        }
        files_[fd] = file;
        return true;
    }

    int32_t remove(std::string path) override
    {
        const std::string full = resolve(path);
        if (full.empty() || full == root_)
            return Lava::False;
        struct stat info {};
        if (::stat(full.c_str(), &info) != 0)
            return Lava::False;
        const int status = S_ISDIR(info.st_mode) ? ::rmdir(full.c_str()) : ::unlink(full.c_str());
        return status == 0 ? Lava::True : Lava::False;
    }

    int32_t makeDir(std::string path) override
    {
        const std::string full = resolve(path);
        if (full.empty())
            return Lava::False;
        if (::mkdir(full.c_str(), 0755) == 0 || errno == EEXIST)
            return Lava::True;
        return Lava::False;
    }

    std::vector<std::string> listDir(std::string path) override
    {
        const std::string full = resolve(path);
        std::vector<std::string> entries;
        DIR *directory = ::opendir(full.c_str());
        if (!directory)
            return entries;
        while (dirent *entry = ::readdir(directory)) {
            const std::string name(entry->d_name);
            if (name == "." || name == ".." || name.size() > 14)
                continue;
            entries.push_back(name);
        }
        ::closedir(directory);
        std::sort(entries.begin(), entries.end());
        return entries;
    }

    int32_t changeDir(std::string path) override
    {
        const std::string virtual_path = normalize_virtual(path);
        const std::string full = root_ + (virtual_path == "/" ? std::string() : virtual_path);
        struct stat info {};
        if (::stat(full.c_str(), &info) != 0 || !S_ISDIR(info.st_mode))
            return Lava::False;
        current_dir_ = virtual_path;
        return Lava::True;
    }

    int32_t exec(std::string path, std::string command_line, uint32_t) override
    {
        const std::string virtual_path = normalize_virtual(path);
        const std::string full = root_ + (virtual_path == "/" ? std::string() : virtual_path);
        struct stat info {};
        if (::stat(full.c_str(), &info) != 0 || !S_ISREG(info.st_mode))
            return -1;
        result_.requested_program = virtual_path;
        result_.command_line = std::move(command_line);
        return 0;
    }

    std::string getCommandLine() override { return command_line_; }

    time_t getTime() override
    {
        std::time_t now = std::time(nullptr);
        struct tm local {};
        localtime_r(&now, &local);
        return time_t{static_cast<uint16_t>(local.tm_year + 1900),
                      static_cast<uint8_t>(local.tm_mon + 1),
                      static_cast<uint8_t>(local.tm_mday),
                      static_cast<uint8_t>(local.tm_hour),
                      static_cast<uint8_t>(local.tm_min),
                      static_cast<uint8_t>(local.tm_sec),
                      static_cast<uint8_t>(local.tm_wday)};
    }

    int32_t delayMs(uint32_t delay) override
    {
        if (delay_deadline_ == 0)
            delay_deadline_ = monotonic_ms() + delay;
        pump_input(1);
        if (result_.host_exit || interrupted)
            return -1;
        if (monotonic_ms() < delay_deadline_)
            return -1;
        delay_deadline_ = 0;
        return 0;
    }

    int32_t getMs() override { return static_cast<int32_t>(monotonic_ms()); }

private:
    static std::string trim_trailing_slash(std::string path)
    {
        while (path.size() > 1 && path.back() == '/')
            path.pop_back();
        return path;
    }

    static bool same_virtual_path(std::string left, std::string right)
    {
        std::replace(left.begin(), left.end(), '\\', '/');
        std::replace(right.begin(), right.end(), '\\', '/');
        return left == right;
    }

    std::string normalize_virtual(std::string path) const
    {
        std::replace(path.begin(), path.end(), '\\', '/');
        std::vector<std::string> parts;
        if (path.empty() || path.front() != '/') {
            size_t start = 0;
            while (start < current_dir_.size()) {
                size_t end = current_dir_.find('/', start + 1);
                std::string part = current_dir_.substr(start + 1, end - start - 1);
                if (!part.empty())
                    parts.push_back(part);
                if (end == std::string::npos)
                    break;
                start = end;
            }
        }

        size_t start = path.front() == '/' ? 1 : 0;
        while (start <= path.size()) {
            size_t end = path.find('/', start);
            std::string part = path.substr(start, end - start);
            if (part.empty() || part == ".") {
                // Nothing.
            } else if (part == "..") {
                if (!parts.empty())
                    parts.pop_back();
            } else {
                parts.push_back(part);
            }
            if (end == std::string::npos)
                break;
            start = end + 1;
        }

        std::string normalized = "/";
        for (size_t i = 0; i < parts.size(); ++i) {
            if (i)
                normalized.push_back('/');
            normalized += parts[i];
        }
        return normalized;
    }

    std::string resolve(const std::string &path) const
    {
        const std::string virtual_path = normalize_virtual(path);
        return root_ + (virtual_path == "/" ? std::string() : virtual_path);
    }

    RunResult run_one(const std::string &program)
    {
        result_ = {};
        delay_deadline_ = 0;
        key_queue_.clear();
        last_key_ = 0;
        pressed_.fill(false);

        Lava vm;
        vm_ = &vm;
        vm.setCallbacks(this);
        vm.loadLvmBin(font_);
        vm.load(read_file(resolve(program)));

        uint64_t operations = 0;
        while (!result_.host_exit && !result_.program_exit &&
               result_.requested_program.empty() && !interrupted) {
            pump_input(0);
            vm.run();
            flush_pending(false);
            if ((++operations & 0x3ff) == 0)
                ::usleep(250);
        }
        vm_ = nullptr;
        return result_;
    }

    void open_inputs()
    {
        const char *paths[] = {"/dev/input/event0", "/dev/input/event1"};
        for (const char *path : paths) {
            int fd = ::open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
            if (fd < 0)
                throw std::runtime_error(std::string("cannot open ") + path + ": " + std::strerror(errno));
            if (::ioctl(fd, EVIOCGRAB, 1) != 0) {
                const int saved_errno = errno;
                ::close(fd);
                for (int opened : input_fds_)
                    ::close(opened);
                input_fds_.clear();
                throw std::runtime_error(std::string("cannot grab ") + path + ": " +
                                         std::strerror(saved_errno));
            }
            input_fds_.push_back(fd);
        }
        pump_input(0);
        key_queue_.clear();
    }

    static uint8_t map_key(uint16_t code, bool shift)
    {
        switch (code) {
        case 16: return shift ? 'Q' : 'q'; case 17: return shift ? 'W' : 'w';
        case 18: return shift ? 'E' : 'e'; case 19: return shift ? 'R' : 'r';
        case 20: return shift ? 'T' : 't'; case 21: return shift ? 'Y' : 'y';
        case 22: return shift ? 'U' : 'u'; case 23: return shift ? 'I' : 'i';
        case 24: return shift ? 'O' : 'o'; case 25: return shift ? 'P' : 'p';
        case 30: return shift ? 'A' : 'a'; case 31: return shift ? 'S' : 's';
        case 32: return shift ? 'D' : 'd'; case 33: return shift ? 'F' : 'f';
        case 34: return shift ? 'G' : 'g'; case 35: return shift ? 'H' : 'h';
        case 36: return shift ? 'J' : 'j'; case 37: return shift ? 'K' : 'k';
        case 38: return shift ? 'L' : 'l'; case 44: return shift ? 'Z' : 'z';
        case 45: return shift ? 'X' : 'x'; case 46: return shift ? 'C' : 'c';
        case 47: return shift ? 'V' : 'v'; case 48: return shift ? 'B' : 'b';
        case 49: return shift ? 'N' : 'n'; case 50: return shift ? 'M' : 'm';
        case 28: return Lava::KeyEnter;
        case 57: return Lava::KeySpace;
        case 103: return Lava::KeyUp;
        case 108: return Lava::KeyDown;
        case 106: return Lava::KeyRight;
        case 105: return Lava::KeyLeft;
        case 114: return Lava::KeyPageUp;
        case 115: return Lava::KeyPageDown;
        case 143: return Lava::KeyHelp;
        case 158: return Lava::KeyEsc;
        case 352: return Lava::KeyEnter;
        default: return 0;
        }
    }

    void pump_input(int timeout_ms)
    {
        flush_pending(false);
        if (input_fds_.empty())
            return;
        std::vector<pollfd> polls(input_fds_.size());
        for (size_t i = 0; i < input_fds_.size(); ++i)
            polls[i] = pollfd{input_fds_[i], POLLIN, 0};
        int status = ::poll(polls.data(), polls.size(), timeout_ms);
        if (status <= 0)
            return;

        for (const pollfd &item : polls) {
            if (!(item.revents & POLLIN))
                continue;
            for (;;) {
                uint8_t event[16];
                const ssize_t count = ::read(item.fd, event, sizeof(event));
                if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                    break;
                if (count != static_cast<ssize_t>(sizeof(event)))
                    break;
                const uint16_t type = event[8] | (event[9] << 8);
                const uint16_t code = event[10] | (event[11] << 8);
                const int32_t value = static_cast<int32_t>(event[12] | (event[13] << 8) |
                                      (event[14] << 16) | (event[15] << 24));
                if (type != 1)
                    continue;
                if (code <= KEY_MAX) {
                    if (value == 0)
                        physical_pressed_[code] = false;
                    else if (value == 1 || value == 2)
                        physical_pressed_[code] = true;
                }
                if (code == 42) {
                    shift_ = value != 0;
                    continue;
                }
                const uint8_t key = map_key(code, shift_);
                if (!key)
                    continue;
                if (value == 0) {
                    pressed_[key] = false;
                    if (last_key_ == key)
                        last_key_ = 0;
                } else if (value == 1 || value == 2) {
                    pressed_[key] = true;
                    last_key_ = key;
                    key_queue_.push_back(key);
                    if (key_queue_.size() > 64)
                        key_queue_.erase(key_queue_.begin());
                }
            }
        }
    }

    void wait_for_key_release()
    {
        // Keep EVIOCGRAB held until the key which ended the app is released.
        // Otherwise its trailing release reaches mpenMain after SIGCONT and
        // can activate the currently selected desktop item.
        const uint64_t timeout = monotonic_ms() + 3000;
        uint64_t quiet_since = 0;
        while (!interrupted && monotonic_ms() < timeout) {
            pump_input(20);
            const bool any_pressed = std::any_of(
                physical_pressed_.begin(), physical_pressed_.end(),
                [](bool pressed) { return pressed; });
            const uint64_t now = monotonic_ms();
            if (any_pressed) {
                quiet_since = 0;
            } else if (quiet_since == 0) {
                quiet_since = now;
            } else if (now - quiet_since >= 120) {
                return;
            }
        }
    }

    void render(const uint8_t *source, int source_width, int source_height,
                LavaDisp::mode_t mode)
    {
        frame_.fill(0);
        // C1-Slim has a strictly bi-level panel.  Preserve the LavaX aspect
        // ratio, scale with nearest-neighbour sampling, and threshold all
        // colour modes directly to black or white.  Ordered dithering looks
        // like grey on LCDs, but becomes noisy checkerboard texture here.
        int target_width = kPanelWidth;
        int target_height = source_height * kPanelWidth / source_width;
        if (target_height > kPanelHeight) {
            target_height = kPanelHeight;
            target_width = source_width * kPanelHeight / source_height;
        }
        const int x_offset = (kPanelWidth - target_width) / 2;
        const int y_offset = (kPanelHeight - target_height) / 2;
        const uint32_t max_value = mode == LavaDisp::GraphicMono ? 1u :
                                   mode == LavaDisp::Graphic16 ? 15u : 255u;

        for (int y = 0; y < target_height; ++y) {
            const int sy = y * source_height / target_height;
            for (int x = 0; x < target_width; ++x) {
                const int sx = x * source_width / target_width;
                const uint32_t value = source[sy * source_width + sx];
                // LavaX uses zero for the white background and increasing
                // values for darker pixels.  The C1 panel uses a set bit for
                // black, so only the dark half of the source range is set.
                // Magic Tower stores its third key in palette index 0.  Keep
                // the workaround inside that fixed status-bar cell; treating
                // index 0 as black globally would turn other coloured status
                // artwork into solid blocks.
                const bool mota_third_key = mota_key_fix_ &&
                                            source_width == 160 && source_height == 80 &&
                                            sx >= 36 && sx <= 44 && sy >= 58 && sy <= 67 &&
                                            value == 0;
                const bool black = value * 2u > max_value ||
                                   (mode == LavaDisp::Graphic16 && mota_third_key);
                if (!black)
                    continue;
                const int px = x + x_offset;
                const int py = y + y_offset;
                frame_[(py / 8) * kPanelWidth + px] |= static_cast<uint8_t>(0x80u >> (py & 7));
            }
        }

        if (frame_ == cached_) {
            pending_frame_valid_ = false;
            return;
        }

        if (stable_frames_) {
            if (!pending_frame_valid_ || frame_ != pending_frame_) {
                pending_frame_ = frame_;
                pending_frame_valid_ = true;
                pending_changed_ms_ = monotonic_ms();
            }
            return;
        }

        write_panel_frame(frame_);
    }

    void flush_pending(bool force)
    {
        if (!stable_frames_ || !pending_frame_valid_)
            return;
        const uint64_t now = monotonic_ms();
        if (!force && (delay_deadline_ != 0 || now - pending_changed_ms_ < kStableFrameMs))
            return;
        write_panel_frame(pending_frame_);
        pending_frame_valid_ = false;
    }

    void write_panel_frame(const std::array<uint8_t, kPanelBytes> &output)
    {
        if (output == cached_)
            return;
        const uint64_t now = monotonic_ms();
        const uint64_t earliest = last_panel_write_ms_ + kMinimumPanelFrameMs;
        if (last_panel_write_ms_ != 0 && now < earliest)
            ::usleep(static_cast<useconds_t>((earliest - now) * 1000u));

        const auto write_panel = [](const std::array<uint8_t, kPanelBytes> &output) {
            const int fd = ::open(kDisplayDevice, O_WRONLY | O_NONBLOCK | O_CLOEXEC);
            if (fd < 0) {
                throw std::runtime_error(std::string("cannot open e-paper display: ") +
                                         std::strerror(errno));
            }
            const ssize_t written = ::write(fd, output.data(), output.size());
            const int saved_errno = errno;
            ::close(fd);
            if (written != static_cast<ssize_t>(output.size())) {
                throw std::runtime_error(std::string("e-paper frame write failed: ") +
                                         std::strerror(saved_errno));
            }
        };

        // Submit only the final target frame.  The launcher wrapper keeps the
        // controller in fast-refresh-only mode while the game is active, so
        // neither an automatic slow waveform nor a visible white interstitial
        // is introduced here.
        write_panel(output);

        cached_ = output;
        cached_frame_valid_ = true;
        last_panel_write_ms_ = monotonic_ms();
    }

    std::string root_;
    std::string current_dir_ = "/";
    std::string command_line_;
    std::vector<uint8_t> font_;
    std::vector<int> input_fds_;
    std::map<unsigned int, FILE *> files_;
    std::array<bool, 256> pressed_{};
    std::array<bool, KEY_MAX + 1> physical_pressed_{};
    std::vector<uint8_t> key_queue_;
    uint8_t last_key_ = 0;
    bool shift_ = false;
    uint64_t delay_deadline_ = 0;
    Lava *vm_ = nullptr;
    RunResult result_;
    std::array<uint8_t, kPanelBytes> frame_{};
    std::array<uint8_t, kPanelBytes> cached_{};
    bool cached_frame_valid_ = false;
    uint64_t last_panel_write_ms_ = 0;
    bool stable_frames_ = false;
    bool mota_key_fix_ = false;
    std::array<uint8_t, kPanelBytes> pending_frame_{};
    bool pending_frame_valid_ = false;
    uint64_t pending_changed_ms_ = 0;
};

} // namespace

int main(int argc, char **argv)
{
    if (argc == 2 && std::string(argv[1]) == "--version") {
        std::cout << "C1LavaX " << kVersion << " standalone\n";
        return 0;
    }
    if (argc > 2) {
        std::cerr << "usage: " << argv[0] << " [virtual-program-path]\n";
        return 2;
    }

    const char *root_env = std::getenv("C1LAVAX_ROOT");
    const char *font_env = std::getenv("C1LAVAX_FONT");
    const char *stable_env = std::getenv("C1LAVAX_STABLE_FRAMES");
    const char *mota_key_env = std::getenv("C1LAVAX_MOTA_KEY_FIX");
    const std::string root = root_env && *root_env ? root_env : kDefaultRoot;
    const std::string font = font_env && *font_env ? font_env : kDefaultFont;
    std::string program = argc == 2 ? argv[1] : "/System/ShellX.sys";

    if (program.compare(0, root.size(), root) == 0)
        program.erase(0, root.size());
    if (program.empty() || program.front() != '/')
        program.insert(program.begin(), '/');

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGHUP, signal_handler);

    try {
        C1Platform platform(root, read_file(font), stable_env && *stable_env &&
                           std::string(stable_env) != "0",
                           mota_key_env && *mota_key_env &&
                           std::string(mota_key_env) != "0");
        return platform.run(program);
    } catch (const std::exception &error) {
        std::cerr << "c1lavax: " << error.what() << '\n';
        return 1;
    }
}
