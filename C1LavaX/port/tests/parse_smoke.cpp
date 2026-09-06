#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "lava.h"

static std::vector<uint8_t> read_file(const std::string &path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("cannot open " + path);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(input),
                                std::istreambuf_iterator<char>());
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        std::cerr << "usage: parse_smoke LVM.bin program.lav\n";
        return 2;
    }
    try {
        Lava vm;
        vm.loadLvmBin(read_file(argv[1]));
        vm.load(read_file(argv[2]));
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
