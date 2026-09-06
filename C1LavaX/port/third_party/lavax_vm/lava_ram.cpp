#include <stdexcept>

#include "lava_ram.h"
#include "lava_cfg.h"

void LavaRam::init(uint32_t rambits)
{
    this->ram_bits = rambits;
    this->ram_mask = rambits <= 16 ? 0x0000ffff : 0x00ffffff;
    if (rambits <= 16)
        ram.resize(1ul << 16);
    else
        ram.resize(1ul << 24);
    std::fill(ram.begin(), ram.end(), 0);

    stack          = LAVA_STACK_OFFSET;
    stack_string   = LAVA_STRING_OFFSET;
    stack_fp_end   = 0;
    stack_fp_start = 0;
}

uint32_t LavaRam::pushString(const std::vector<uint8_t> &dstr)
{
    uint32_t str = stack_string;
    std::copy(dstr.begin(), dstr.end(), ram.begin() + stack_string);
    stack_string += dstr.size();
    if (stack_string >= LAVA_STRING_OFFSET + LAVA_STRING_LIMIT)
        stack_string = LAVA_STRING_OFFSET;
    return str;
}
