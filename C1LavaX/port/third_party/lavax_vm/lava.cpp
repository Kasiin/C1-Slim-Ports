#include <iostream>
#include <stdexcept>

#include "lava.h"

Lava::Lava()
{
    proc.setDisp(&disp);
}

void Lava::load(const std::vector<uint8_t> &source)
{
    // Check 16-byte file header
    const uint8_t *header = source.data();
    if (source.size() < 16) {
        std::cerr << "File too short" << std::endl;
        return;
    }

    // File signature
    if (!(header[0] == 'L' && header[1] == 'A' && header[2] == 'V' && header[3] == 0x12)) {
        std::cerr << "File signature mismatch" << std::endl;
        return;
    }

    // Configurations
    uint8_t cfg = header[8];

    bool pen_input = cfg & 0x01;

    // Match the original LavaX VM: bit 0x10 takes precedence when both
    // address-width flags are present.  Shell.sys uses cfg=0xf0, so treating
    // 0x90 as a separate enum value incorrectly rejects the stock shell.
    uint32_t rambits = 16;
    if (cfg & 0x10) {
        rambits = 32;
    } else if (cfg & 0x80) {
        rambits = 24;
    }

    LavaDisp::mode_t gmode;
    switch (cfg & 0x60) {
    case 0x00:
        gmode = LavaDisp::GraphicMono;
        break;
    case 0x40:
        gmode = LavaDisp::Graphic16;
        break;
    case 0x60:
        gmode = LavaDisp::Graphic256;
        break;
    default:
        std::cerr << "Unknown graphic mode: " << std::to_string(cfg) << std::endl;
        return;
    }

    // Screen size
    uint32_t w = header[9] * 16;
    uint32_t h = header[10] * 16;
    w = w >= 320 ? 320 : w < 160 ? 160 : w;
    h = h >= 240 ? 240 : h <  80 ?  80 : h;
    std::cerr << "Screen size: " << w << "x" << h << std::endl;

    disp.setMode(gmode);
    disp.setSize(w, h);
    disp.clearScreen();

    proc.load(source, rambits, pen_input);
}

void Lava::reset()
{
    disp.setMode(disp.getMode());
    disp.clearScreen();
    proc.reset();
}

void Lava::saveState(std::ostream &ss)
{
    const uint32_t version = 0;
    ss.put(version >>  0);
    ss.put(version >>  8);
    ss.put(version >> 16);
    ss.put(version >> 24);

    disp.saveState(ss);
    proc.saveState(ss);
}

void Lava::restoreState(std::istream &ss)
{
    uint32_t version = 0;
    version |= ss.get() <<  0;
    version |= ss.get() <<  8;
    version |= ss.get() << 16;
    version |= ss.get() << 24;

    disp.restoreState(ss);
    proc.restoreState(ss);
}
