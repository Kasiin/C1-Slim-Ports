#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "lava.h"

namespace {

std::vector<uint8_t> read_file(const std::string &path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("cannot open " + path);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(input),
                                std::istreambuf_iterator<char>());
}

std::string safe_path(const std::string &base, std::string path)
{
    std::replace(path.begin(), path.end(), '\\', '/');
    while (!path.empty() && path.front() == '/')
        path.erase(path.begin());
    if (path.find("../") != std::string::npos || path == "..")
        return std::string();
    return base + "/" + path;
}

class SmokeCallback final : public LavaCallback {
public:
    SmokeCallback(Lava &vm, std::string base, size_t automatic_keys = 0)
        : vm_(vm), base_(std::move(base)), automatic_keys_(automatic_keys) {}

    void refresh(const uint8_t *framebuffer) override
    {
        const size_t size = static_cast<size_t>(vm_.getFramebufferStride()) *
                            vm_.getFramebufferHeight();
        frame_.assign(framebuffer, framebuffer + size);
        frames_.push_back(frame_);
        ++refreshes_;
    }

    void exit(uint32_t code) override
    {
        stopped_ = true;
        exit_code_ = code;
    }

    int32_t getchar() override
    {
        if (automatic_keys_ != 0) {
            --automatic_keys_;
            return Lava::KeyEnter;
        }
        waiting_ = true;
        return -1;
    }
    int32_t checkKey(uint8_t) override { return Lava::False; }
    int32_t inKey() override { return 0; }
    void releaseKey(uint8_t) override {}
    int32_t debugContinue() override { return 0; }

    uint8_t fopen(std::string path, std::string mode) override
    {
        const std::string full = safe_path(base_, std::move(path));
        if (full.empty())
            return 0;
        std::string clean_mode;
        for (const char c : mode) {
            if (c == 'r' || c == 'w' || c == 'a' || c == '+')
                clean_mode.push_back(c);
        }
        clean_mode.push_back('b');
        for (unsigned int candidate = 0x80; candidate <= 0xff; ++candidate) {
            if (files_.count(candidate) != 0)
                continue;
            FILE *file = std::fopen(full.c_str(), clean_mode.c_str());
            if (!file)
                return 0;
            files_[candidate] = file;
            return static_cast<uint8_t>(candidate);
        }
        return 0;
    }

    void fclose(uint8_t fd) override
    {
        const auto it = files_.find(fd);
        if (it == files_.end())
            return;
        std::fclose(it->second);
        files_.erase(it);
    }

    std::vector<uint8_t> fread(uint8_t fd, uint32_t size) override
    {
        const auto it = files_.find(fd);
        if (it == files_.end())
            return {};
        std::vector<uint8_t> bytes(size);
        bytes.resize(std::fread(bytes.data(), 1, size, it->second));
        return bytes;
    }

    int32_t fwrite(uint8_t fd, const std::vector<uint8_t> &data) override
    {
        const auto it = files_.find(fd);
        if (it == files_.end())
            return 0;
        return static_cast<int32_t>(std::fwrite(data.data(), 1, data.size(), it->second));
    }

    int32_t fseek(uint8_t fd, int32_t offset, fseek_mode_t mode) override
    {
        const auto it = files_.find(fd);
        if (it == files_.end())
            return -1;
        const int whence = mode == SeekCur ? SEEK_CUR : mode == SeekEnd ? SEEK_END : SEEK_SET;
        if (std::fseek(it->second, offset, whence) != 0)
            return -1;
        return static_cast<int32_t>(std::ftell(it->second));
    }

    int32_t ftell(uint8_t fd) override
    {
        const auto it = files_.find(fd);
        return it == files_.end() ? -1 : static_cast<int32_t>(std::ftell(it->second));
    }
    bool feof(uint8_t fd) override
    {
        const auto it = files_.find(fd);
        if (it == files_.end())
            return true;
        const long position = std::ftell(it->second);
        if (position < 0 || std::fseek(it->second, 0, SEEK_END) != 0)
            return true;
        const long end = std::ftell(it->second);
        (void)std::fseek(it->second, position, SEEK_SET);
        return position >= end;
    }

    bool frestore(uint8_t, std::string, std::string, int32_t) override { return false; }
    int32_t remove(std::string) override { return 0; }
    int32_t makeDir(std::string path) override
    {
        const std::string full = safe_path(base_, std::move(path));
        if (full.empty())
            return Lava::False;
        std::error_code error;
        return std::filesystem::create_directories(full, error) && !error
            ? Lava::True : Lava::False;
    }
    std::vector<std::string> listDir(std::string path) override
    {
        const std::string full = safe_path(base_, std::move(path));
        std::vector<std::string> entries;
        std::error_code error;
        for (const auto &entry : std::filesystem::directory_iterator(full, error)) {
            const std::string name = entry.path().filename().string();
            if (name.size() <= 14)
                entries.push_back(name);
        }
        std::sort(entries.begin(), entries.end());
        return entries;
    }
    int32_t changeDir(std::string path) override
    {
        const std::string full = safe_path(base_, std::move(path));
        std::error_code error;
        return std::filesystem::is_directory(full, error) && !error
            ? Lava::True : Lava::False;
    }
    int32_t exec(std::string, std::string, uint32_t) override { return -1; }
    std::string getCommandLine() override { return {}; }
    time_t getTime() override { return {}; }
    int32_t delayMs(uint32_t) override { return 0; }
    int32_t getMs() override { return 0; }

    bool waiting() const { return waiting_; }
    bool stopped() const { return stopped_; }
    uint32_t exit_code() const { return exit_code_; }
    size_t refreshes() const { return refreshes_; }
    const std::vector<uint8_t> &frame() const { return frame_; }
    const std::vector<std::vector<uint8_t>> &frames() const { return frames_; }

private:
    Lava &vm_;
    std::string base_;
    std::unordered_map<unsigned int, FILE *> files_;
    std::vector<uint8_t> frame_;
    std::vector<std::vector<uint8_t>> frames_;
    size_t automatic_keys_ = 0;
    size_t refreshes_ = 0;
    uint32_t exit_code_ = 0;
    bool waiting_ = false;
    bool stopped_ = false;
};

void write_pgm(const std::string &path, Lava &vm, const std::vector<uint8_t> &frame)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << "P5\n" << vm.getFramebufferWidth() << ' ' << vm.getFramebufferHeight()
           << "\n255\n";
    const unsigned int mask = vm.getGraphicMode() == LavaDisp::GraphicMono ? 1 :
                              vm.getGraphicMode() == LavaDisp::Graphic16 ? 15 : 255;
    for (const uint8_t pixel : frame) {
        const uint8_t gray = static_cast<uint8_t>(255u * pixel / mask);
        output.put(static_cast<char>(gray));
    }
}

void write_bmp(const std::string &path, Lava &vm, const std::vector<uint8_t> &frame)
{
    const uint32_t width = vm.getFramebufferWidth();
    const uint32_t height = vm.getFramebufferHeight();
    const uint32_t row_size = (width * 3u + 3u) & ~3u;
    const uint32_t image_size = row_size * height;
    const uint32_t file_size = 54u + image_size;
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    auto word = [&](uint16_t value) {
        output.put(static_cast<char>(value));
        output.put(static_cast<char>(value >> 8));
    };
    auto dword = [&](uint32_t value) {
        word(static_cast<uint16_t>(value));
        word(static_cast<uint16_t>(value >> 16));
    };
    output.write("BM", 2);
    dword(file_size); dword(0); dword(54);
    dword(40); dword(width); dword(height); word(1); word(24);
    dword(0); dword(image_size); dword(2835); dword(2835); dword(0); dword(0);
    const unsigned int mask = vm.getGraphicMode() == LavaDisp::GraphicMono ? 1 :
                              vm.getGraphicMode() == LavaDisp::Graphic16 ? 15 : 255;
    for (uint32_t row = 0; row < height; ++row) {
        const uint32_t y = height - 1u - row;
        for (uint32_t x = 0; x < width; ++x) {
            const uint8_t gray = static_cast<uint8_t>(255u * frame[y * width + x] / mask);
            output.put(static_cast<char>(gray));
            output.put(static_cast<char>(gray));
            output.put(static_cast<char>(gray));
        }
        for (uint32_t pad = width * 3u; pad < row_size; ++pad)
            output.put('\0');
    }
}

} // namespace

int main(int argc, char **argv)
{
    if (argc != 5 && argc != 6) {
        std::cerr << "usage: core_smoke LVM.bin program.lav base output[.pgm|.bmp] [automatic-keys]\n";
        return 2;
    }
    try {
        static Lava vm;
        const size_t automatic_keys = argc == 6 ? std::stoul(argv[5]) : 0;
        SmokeCallback callback(vm, argv[3], automatic_keys);
        vm.setCallbacks(&callback);
        vm.loadLvmBin(read_file(argv[1]));
        vm.load(read_file(argv[2]));
        for (size_t operations = 0; operations < 5000000 && !callback.stopped(); ++operations) {
            vm.run();
            if (callback.waiting() && callback.refreshes() != 0)
                break;
        }
        if (callback.frame().empty())
            throw std::runtime_error("VM did not produce a framebuffer");
        const std::string output_path = argv[4];
        if (argc == 6) {
            size_t index = 0;
            for (const auto &frame : callback.frames()) {
                char suffix[32];
                std::snprintf(suffix, sizeof(suffix), "-%03zu.bmp", ++index);
                write_bmp(output_path + suffix, vm, frame);
            }
        } else if (output_path.size() >= 4 && output_path.substr(output_path.size() - 4) == ".bmp") {
            write_bmp(output_path, vm, callback.frame());
        } else {
            write_pgm(output_path, vm, callback.frame());
        }
        std::cout << "width=" << vm.getFramebufferWidth()
                  << " height=" << vm.getFramebufferHeight()
                  << " mode=" << static_cast<int>(vm.getGraphicMode())
                  << " refreshes=" << callback.refreshes()
                  << " stopped=" << callback.stopped()
                  << " exit=" << callback.exit_code() << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "core_smoke: " << error.what() << '\n';
        return 1;
    }
}
