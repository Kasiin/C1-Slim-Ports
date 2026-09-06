#pragma once

#include <cstdint>
#include <vector>

#include "lava_cfg.h"

class LavaDisp
{
public:
    LavaDisp();
    void loadLvmBin(const std::vector<uint8_t> &data) {this->lvm_data = data;}

    enum mode_t {
        GraphicMono    = 1,
        Graphic16      = 4,
        Graphic256     = 8,
    };

    void setSize(uint16_t w, uint16_t h);
    void setMode(mode_t mode);
    mode_t getMode() {return graphic_mode;}

    uint8_t refreshRequest()
    {
        uint8_t v = refresh;
        refresh = 0;
        return v;
    }

    void saveState(std::ostream &ss);
    void restoreState(std::istream &ss);

    void clearScreen();
    void clearWorking();

    // Draw solid rectangle block
    void drawBlock(int16_t x0, int16_t x1, int16_t y0, int16_t y1, uint8_t cfg);
    // Draw block
    void drawBlock(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t cfg,
                   const std::vector<uint8_t> &data);
    // Draw rectangle
    void drawRectangle(uint16_t x0, uint16_t x1, uint16_t y0, uint16_t y1, uint8_t cfg);
    void drawPoint(int16_t x, int16_t y, uint8_t cfg);
    uint8_t getPoint(int16_t x, int16_t y, uint8_t cfg);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t cfg);
    void drawEllipse(int16_t x, int16_t y, uint16_t rx, uint16_t ry, bool filled, uint8_t cfg);
    // Draw text
    void drawText(const std::vector<uint8_t> &str, int16_t x, int16_t y, uint8_t cfg);
    // xdraw
    void xdraw(uint8_t cmd);
    void fade(uint8_t amount);

    // Read framebuffer data
    std::vector<uint8_t> getBlock(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t cfg);

    void setForegroundColour(uint8_t c) {fg_colour = c & colour_mask;}
    void setBackgroundColour(uint8_t c) {bg_colour = c & colour_mask;}

    const uint8_t *getFramebuffer() {return fbData(false, 0, 0);}
    const uint8_t *getWorkingFramebuffer() {return fbData(true, 0, 0);}
    void framebufferFlush();
    uint16_t getFramebufferWidth() {return width;}
    uint16_t getFramebufferHeight() {return height;}
    uint16_t getFramebufferStride() {return width;}

private:
    // Draw a horizontal line
    void drawHLine(int16_t x, int16_t y, uint16_t w, uint8_t cfg);
    // Draw a vertical line
    void drawVLine(int16_t x, int16_t y, uint16_t h, uint8_t cfg);
    // Draw a character
    void drawCharacter(uint16_t c, int16_t x, int16_t y, uint8_t cfg);
    // Draw block, with 1 bbp source data
    void drawBlockMono(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t cfg,
                       const std::vector<uint8_t> &data);

    void framebufferFill(uint8_t c);
    void framebufferClear();

    uint8_t *fbData(bool working, int y, int x)
    {
        return working ? &fb_working[y * width + x] : &fb_disp[y * width + x];
    }

    bool inRange(int32_t x, int32_t y) {return x >= 0 && y >= 0 && x < width && y < height;}

    std::vector<uint8_t> lvm_data;
    std::vector<uint8_t> fb_disp, fb_working;

    uint16_t width, height;
    mode_t graphic_mode;
    uint8_t fg_colour, bg_colour, colour_mask;
    uint8_t refresh;
};
