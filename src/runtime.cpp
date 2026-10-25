#include "runtime.hpp"
#include "font5x7.hpp"

#include <SDL2/SDL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

namespace rt {

u8 gA, gX, gY, gS;
u8 gC, gZ, gN, gV, gI, gD;
u8 gBank;
u16 gPC = 0;
int gRomBanks = 1;
u8 gRAM[0x100];
const u8* gRomBanksPtr[2] = {nullptr, nullptr};

namespace {

// ---------------------------------------------------------------------------
// Video constants (PAL 2600: 312 lines, 228 color clocks per line)
// ---------------------------------------------------------------------------
constexpr int VIS_START = 68;
constexpr int VIS_WIDTH = 160;
constexpr int LINE_CLOCKS = 228;
constexpr int MAX_LINES = 400;
constexpr int FRAME_HEIGHT = 312;
constexpr int DISPLAY_TOP = 60;
constexpr int DISPLAY_BOTTOM = 52;
constexpr int LOGICAL_HEIGHT = FRAME_HEIGHT - DISPLAY_TOP - DISPLAY_BOTTOM;
constexpr int TEXT_STEP = 6;
constexpr int TEXT_LINE = 9;

static const u32 sPalette[256] = {
    0x0B0B0B, 0x0B0B0B, 0x333333, 0x333333, 0x595959, 0x595959, 0x7B7B7B, 0x7B7B7B,
    0x8B8B8B, 0x8B8B8B, 0xAAAAAA, 0xAAAAAA, 0xC7C7C7, 0xC7C7C7, 0xE3E3E3, 0xE3E3E3,
    0x000000, 0x000000, 0x272727, 0x272727, 0x404040, 0x404040, 0x696969, 0x696969,
    0x8B8B8B, 0x8B8B8B, 0xAAAAAA, 0xAAAAAA, 0xC7C7C7, 0xC7C7C7, 0xE3E3E3, 0xE3E3E3,
    0x3B2400, 0x272727, 0x664700, 0x484848, 0x8B7000, 0x6B6B6B, 0xAC9200, 0x898989,
    0xC5AE36, 0xA7A7A7, 0xDEC85E, 0xC2C2C2, 0xF7E27F, 0xDDDDDD, 0xFFF19E, 0xECECEC,
    0x004500, 0x292929, 0x006F00, 0x414141, 0x3B9200, 0x676767, 0x65B009, 0x878787,
    0x85CA3D, 0xA5A5A5, 0xA3E364, 0xC1C1C1, 0xBFFC84, 0xDCDCDC, 0xD5FFA5, 0xE8E8E8,
    0x590000, 0x1B1B1B, 0x802700, 0x3D3D3D, 0xA15700, 0x636363, 0xBC7937, 0x868686,
    0xD6985F, 0xA4A4A4, 0xEEB381, 0xBFBFBF, 0xFFCE9E, 0xD7D7D7, 0xFFDCBD, 0xE3E3E3,
    0x004900, 0x2B2B2B, 0x007200, 0x434343, 0x169216, 0x5F5F5F, 0x45AF45, 0x838383,
    0x6BC96B, 0xA2A2A2, 0x8BE38B, 0xBFBFBF, 0xA9FBA9, 0xD9D9D9, 0xC5FFC5, 0xE7E7E7,
    0x640012, 0x202020, 0x890821, 0x313131, 0xA73D4D, 0x5F5F5F, 0xC26472, 0x828282,
    0xDC8491, 0xA0A0A0, 0xF4A3AE, 0xBCBCBC, 0xFFBECA, 0xD3D3D3, 0xFFDAE0, 0xE6E6E6,
    0x003D29, 0x282828, 0x006A48, 0x464646, 0x048E63, 0x606060, 0x3CAA84, 0x858585,
    0x62C5A2, 0xA3A3A3, 0x83DFBE, 0xC0C0C0, 0xA1F8D9, 0xDADADA, 0xBEFFE9, 0xE9E9E9,
    0x550046, 0x212121, 0x88006E, 0x353535, 0xA5318D, 0x5E5E5E, 0xC159AA, 0x818181,
    0xDA7CC5, 0xA0A0A0, 0xF39ADF, 0xBCBCBC, 0xFFB9F3, 0xD5D5D5, 0xFFD4F6, 0xE5E5E5,
    0x003651, 0x292929, 0x005A7D, 0x434343, 0x117E9C, 0x616161, 0x429CB8, 0x848484,
    0x68B7D2, 0xA2A2A2, 0x88D2EB, 0xBFBFBF, 0xA6EBFF, 0xD9D9D9, 0xC3FFFF, 0xEDEDED,
    0x4C007C, 0x252525, 0x75009D, 0x353535, 0x932EB8, 0x5C5C5C, 0xAF57D2, 0x7F7F7F,
    0xCA7AEB, 0x9F9F9F, 0xE499FF, 0xBBBBBB, 0xECB7FF, 0xCFCFCF, 0xF3D4FF, 0xE2E2E2,
    0x002D83, 0x292929, 0x003EA4, 0x373737, 0x2D65BF, 0x5F5F5F, 0x5685DA, 0x818181,
    0x79A2F2, 0x9F9F9F, 0x99BFFF, 0xBBBBBB, 0xB7DBFF, 0xD4D4D4, 0xD3F5FF, 0xECECEC,
    0x220096, 0x1B1B1B, 0x5200B6, 0x2D2D2D, 0x7538CF, 0x5B5B5B, 0x945FE8, 0x7E7E7E,
    0xB181FF, 0x9E9E9E, 0xC5A0FF, 0xB6B6B6, 0xD6BDFF, 0xCCCCCC, 0xE8DAFF, 0xE2E2E2,
    0x00009A, 0x121212, 0x241DB6, 0x313131, 0x504AD0, 0x5B5B5B, 0x746FE9, 0x7E7E7E,
    0x928EFF, 0x9C9C9C, 0xB1ADFF, 0xB8B8B8, 0xCECAFF, 0xD1D1D1, 0xE9E5FF, 0xE9E9E9,
    0x0B0B0B, 0x0B0B0B, 0x333333, 0x333333, 0x595959, 0x595959, 0x7B7B7B, 0x7B7B7B,
    0x999999, 0x999999, 0xB6B6B6, 0xB6B6B6, 0xCFCFCF, 0xCFCFCF, 0xE6E6E6, 0xE6E6E6,
    0x0B0B0B, 0x0B0B0B, 0x333333, 0x333333, 0x595959, 0x595959, 0x7B7B7B, 0x7B7B7B,
    0x999999, 0x999999, 0xB6B6B6, 0xB6B6B6, 0xCFCFCF, 0xCFCFCF, 0xE6E6E6, 0xE6E6E6,
};
u32 sFrame[MAX_LINES][VIS_WIDTH];
u32 sDisplay[LOGICAL_HEIGHT][VIS_WIDTH];
int sFrameLines = FRAME_HEIGHT;
int sVisibleStartLine = -1;
int sLine = 0;
int sClock = 0;
int sFrames = 0;
u64 sCycles = 0;
bool sHadVsync = false;

int sArgc = 0;
char** sArgv = nullptr;
bool sHeadless = false;
long sTargetFrames = -1;
std::string sPpmPath;

bool sSdlReady = false;
SDL_Window* sWindow = nullptr;
SDL_Renderer* sRenderer = nullptr;
SDL_Texture* sTexture = nullptr;
SDL_AudioDeviceID sAudioDev = 0;

bool sQuit = false;
u64 sTickCalls = 0;
int sLogFrame = -1;
int sTraceFrame = -1;
bool sTraceAll = false;
u64 sTraceAllLimit = 0;
int sFrameWsync = 0;
int sFrameTimers = 0;
u64 sFrameTickStart = 0;

bool sHelpVisible = false;
bool sConsoleVisible = false;
bool sPaused = false;
bool sPaceEnabled = true;
bool sPaceInit = false;
using FrameClock = std::chrono::steady_clock;
FrameClock::time_point sNextFrame;
FrameClock::time_point sLastFrame;
double sFps = 0.0;
int sInputMask = -1;

constexpr int LOG_CAP = 64;
std::string sLog[LOG_CAP];
int sLogHead = 0;
int sLogCount = 0;

void logEvent(const char* fmt, ...) {
    char msg[128];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    char line[160];
    std::snprintf(line, sizeof(line), "f%d %s", sFrames, msg);
    sLog[sLogHead] = line;
    sLogHead = (sLogHead + 1) % LOG_CAP;
    if (sLogCount < LOG_CAP) sLogCount++;
}

void pollEvents();

// ---------------------------------------------------------------------------
// TIA
// ---------------------------------------------------------------------------
struct TIAState {
    u8 vsync = 0, vblank = 0;
    u8 nusiz[2] = {0, 0};
    u8 colup[2] = {0, 0};
    u8 colupf = 0, colubk = 0;
    u8 ctrlpf = 0;
    u8 refp[2] = {0, 0};
    u8 pf0 = 0, pf1 = 0, pf2 = 0;
    u8 grpWritten[2] = {0, 0};
    u8 grpDisp[2] = {0, 0};
    bool vdelp[2] = {false, false};
    bool vdelbl = false;
    u8 enam[2] = {0, 0};
    u8 enabl = 0, enablPrev = 0;
    u8 hmp[2] = {0, 0}, hmm[2] = {0, 0}, hmbl = 0;
    u8 resmp[2] = {0, 0};
    int pos[2] = {0, 0};
    int mpos[2] = {0, 0};
    int bpos = 0;
    int off[2] = {0, 0}, moff[2] = {0, 0}, boff = 0;
    u8 coll[8] = {0};
    u8 audc[2] = {0, 0}, audf[2] = {0, 0}, audv[2] = {0, 0};
};

TIAState tia;

// input
u8 sSwcha = 0xFF;
constexpr u8 SWCHB_DEFAULT = 0xCB;
u8 sSwchbBase = SWCHB_DEFAULT; // reset=1 select=1 colour=1 difficulty=1/1
int sResetFrame = -1;
int sSelectFrame = -1;
int sAutoStartFrame = 60;
u8 sInpt4 = 0x80;
u8 sInpt5 = 0x80;

int beamX() { return sClock - VIS_START; }

u32 paletteColor(u8 c) { return sPalette[c]; }

struct CopyInfo { int count; int spacing; int width; };

CopyInfo copyInfo(u8 n) {
    switch (n & 7) {
        case 0: return {1, 0, 8};
        case 1: return {2, 16, 8};
        case 2: return {2, 32, 8};
        case 3: return {3, 16, 8};
        case 4: return {2, 64, 8};
        case 5: return {1, 0, 16};
        case 6: return {3, 32, 8};
        default: return {1, 0, 32};
    }
}

bool playerPixel(int idx, int x) {
    u8 nusiz = tia.nusiz[idx];
    u8 grp = tia.grpDisp[idx];
    if (!grp) return false;
    CopyInfo ci = copyInfo(nusiz);
    int pos = tia.pos[idx] + tia.off[idx];
    int scale = ci.width / 8;
    for (int k = 0; k < ci.count; k++) {
        int dx = x - (pos + k * ci.spacing);
        if (dx >= 0 && dx < ci.width) {
            int bit = dx / scale;
            int b = tia.refp[idx] ? bit : (7 - bit);
            if ((grp >> b) & 1) return true;
        }
    }
    return false;
}

bool missilePixel(int idx, int x) {
    if (!tia.enam[idx]) return false;
    int width = 1 << ((tia.nusiz[idx] >> 4) & 3);
    CopyInfo ci = copyInfo(tia.nusiz[idx]);
    int pos = tia.mpos[idx] + tia.moff[idx];
    for (int k = 0; k < ci.count; k++) {
        int dx = x - (pos + k * ci.spacing);
        if (dx >= 0 && dx < width) return true;
    }
    return false;
}

bool ballPixel(int x) {
    u8 en = tia.vdelbl ? tia.enablPrev : tia.enabl;
    if (!en) return false;
    int width = 1 << ((tia.ctrlpf >> 4) & 3);
    int dx = x - (tia.bpos + tia.boff);
    return dx >= 0 && dx < width;
}

bool playfieldPixel(int x) {
    int b = x >> 2;
    bool reflect = (tia.ctrlpf & 0x01) != 0;
    if (b >= 20) b = reflect ? (39 - b) : (b - 20);
    int bit;
    if (b < 4) {
        bit = (tia.pf0 >> (4 + b)) & 1;
    } else if (b < 12) {
        bit = (tia.pf1 >> (7 - (b - 4))) & 1;
    } else {
        bit = (tia.pf2 >> (b - 12)) & 1;
    }
    return bit != 0;
}

constexpr u8 F_PF = 1, F_BL = 2, F_M0 = 4, F_M1 = 8, F_P0 = 16, F_P1 = 32;

void addCollisions(u8 f) {
    bool pf = f & F_PF, bl = f & F_BL, m0 = f & F_M0, m1 = f & F_M1;
    bool p0 = f & F_P0, p1 = f & F_P1;
    if (m0 && p0) tia.coll[0] |= 0x40;
    if (m0 && p1) tia.coll[0] |= 0x80;
    if (m1 && p0) tia.coll[1] |= 0x80;
    if (m1 && p1) tia.coll[1] |= 0x40;
    if (p0 && pf) tia.coll[2] |= 0x80;
    if (p0 && bl) tia.coll[2] |= 0x40;
    if (p1 && pf) tia.coll[3] |= 0x80;
    if (p1 && bl) tia.coll[3] |= 0x40;
    if (m0 && pf) tia.coll[4] |= 0x80;
    if (m0 && bl) tia.coll[4] |= 0x40;
    if (m1 && pf) tia.coll[5] |= 0x80;
    if (m1 && bl) tia.coll[5] |= 0x40;
    if (bl && pf) tia.coll[6] |= 0x80;
    if (bl && p0) tia.coll[6] |= 0x40;
    if (bl && p1) tia.coll[6] |= 0x20;
    if (p0 && p1) tia.coll[7] |= 0x80;
    if (m0 && m1) tia.coll[7] |= 0x40;
}

u32 renderPixel(int x) {
    u8 f = 0;
    if (playfieldPixel(x)) f |= F_PF;
    if (ballPixel(x)) f |= F_BL;
    if (missilePixel(0, x)) f |= F_M0;
    if (missilePixel(1, x)) f |= F_M1;
    if (playerPixel(0, x)) f |= F_P0;
    if (playerPixel(1, x)) f |= F_P1;
    addCollisions(f);

    u8 pfColor = (tia.ctrlpf & 0x02) ? ((x < 80) ? tia.colup[0] : tia.colup[1]) : tia.colupf;
    bool pfOn = (f & (F_PF | F_BL)) != 0;
    bool priority = (tia.ctrlpf & 0x04) != 0;

    u32 c;
    if (!priority) {
        c = paletteColor(tia.colubk);
        if (pfOn) c = paletteColor(pfColor);
        if (f & (F_M0 | F_P0)) c = paletteColor(tia.colup[0]);
        if (f & (F_M1 | F_P1)) c = paletteColor(tia.colup[1]);
    } else {
        c = paletteColor(tia.colubk);
        if (f & (F_M0 | F_P0)) c = paletteColor(tia.colup[0]);
        if (f & (F_M1 | F_P1)) c = paletteColor(tia.colup[1]);
        if (pfOn) c = paletteColor(pfColor);
    }
    if (tia.vblank & 0x02) c = 0xFF000000u;
    return c;
}

void grpWrite(int idx, u8 v) {
    tia.grpDisp[idx] = tia.vdelp[idx] ? tia.grpWritten[idx] : v;
    tia.grpWritten[idx] = v;
}

int hmoveShift(u8 h) {
    int v = (h >> 4) & 0x0F;
    return (v <= 7) ? -v : 16 - v;
}

u8 mixChannel(u8 bg, u8 fg, int a) {
    return (u8)((fg * a + bg * (255 - a) + 127) / 255);
}

void blendPixel(int x, int y, u32 fg, int a) {
    if (x < 0 || x >= VIS_WIDTH || y < 0 || y >= LOGICAL_HEIGHT) return;
    u32 bg = sDisplay[y][x];
    u8 r = mixChannel((u8)(bg >> 16), (u8)(fg >> 16), a);
    u8 g = mixChannel((u8)(bg >> 8), (u8)(fg >> 8), a);
    u8 b = mixChannel((u8)bg, (u8)fg, a);
    sDisplay[y][x] = 0xFF000000u | ((u32)r << 16) | ((u32)g << 8) | b;
}

void putPixel(int x, int y, u32 color) {
    if (x < 0 || x >= VIS_WIDTH || y < 0 || y >= LOGICAL_HEIGHT) return;
    sDisplay[y][x] = color;
}

void blendRect(int x, int y, int w, int h, u32 color, int alpha) {
    for (int py = y; py < y + h; py++)
        for (int px = x; px < x + w; px++) blendPixel(px, py, color, alpha);
}

void drawRect(int x, int y, int w, int h, u32 color) {
    for (int px = x; px < x + w; px++) {
        putPixel(px, y, color);
        putPixel(px, y + h - 1, color);
    }
    for (int py = y; py < y + h; py++) {
        putPixel(x, py, color);
        putPixel(x + w - 1, py, color);
    }
}

void drawChar(int x, int y, unsigned char c, u32 color) {
    if (c < 32 || c > 126) c = '?';
    const uint8_t* glyph = FONT5X7[c - 32];
    for (int col = 0; col < 5; col++) {
        u8 bits = glyph[col];
        for (int row = 0; row < 8; row++)
            if (bits & (1u << row)) putPixel(x + col, y + row, color);
    }
}

void drawText(int x, int y, const char* text, u32 color) {
    for (; *text; text++, x += TEXT_STEP) drawChar(x, y, (unsigned char)*text, color);
}

int textWidth(const char* text) {
    int n = (int)std::strlen(text);
    return n > 0 ? n * TEXT_STEP - 1 : 0;
}

void drawTextCentered(int y, const char* text, u32 color) {
    drawText((VIS_WIDTH - textWidth(text)) / 2, y, text, color);
}

void drawHelpOverlay() {
    blendRect(4, 6, 152, 160, 0xFF000000, 150);
    drawRect(4, 6, 152, 160, 0xFF7B7B7B);
    drawTextCentered(14, "ADVENTURE", 0xFFDEC85E);
    drawTextCentered(24, "VCSRelink (PAL)", 0xFFAAAAAA);
    drawText(11, 42, "MOVE      Arrows/WASD", 0xFFE3E3E3);
    drawText(11, 51, "FIRE      Space/Z", 0xFFE3E3E3);
    drawText(11, 60, "SELECT    Tab", 0xFFE3E3E3);
    drawText(11, 69, "START     Enter/Backspace", 0xFFE3E3E3);
    drawText(11, 87, "F1   Toggle this help", 0xFF85CA3D);
    drawText(11, 96, "F2   Debug console", 0xFF85CA3D);
    drawText(11, 105, "P    Pause / resume", 0xFF85CA3D);
    drawText(11, 114, "Esc  Close overlays", 0xFF85CA3D);
    drawText(11, 123, "Esc  Quit game", 0xFF85CA3D);
    drawText(11, 141, "Adventure (PAL), 1980", 0xFFAAAAAA);
    drawText(11, 150, "Warren Robinett / Atari", 0xFFAAAAAA);
}

void drawConsoleOverlay() {
    blendRect(0, 0, VIS_WIDTH, LOGICAL_HEIGHT, 0xFF000000, 205);
    drawRect(2, 2, VIS_WIDTH - 4, LOGICAL_HEIGHT - 4, 0xFF333333);
    int x = 5;
    int y = 4;
    const int step = 8;
    char line[160];
    drawText(x, y, "DEBUG CONSOLE   F2 CLOSE", 0xFFDEC85E); y += step;
    std::snprintf(line, sizeof(line), "FPS %.1f  FRAME %d", sFps, sFrames);
    drawText(x, y, line, 0xFFE3E3E3); y += step;
    std::snprintf(line, sizeof(line), "PC %04X  BANK %d", gPC, gBank);
    drawText(x, y, line, 0xFFE3E3E3); y += step;
    std::snprintf(line, sizeof(line), "CYC %llu", (unsigned long long)sCycles);
    drawText(x, y, line, 0xFFE3E3E3); y += step;
    std::snprintf(line, sizeof(line), "A %02X X %02X Y %02X S %02X", gA, gX, gY, gS);
    drawText(x, y, line, 0xFFE3E3E3); y += step;
    std::snprintf(line, sizeof(line), "FLAGS N%d V%d D%d I%d Z%d C%d", gN, gV, gD, gI, gZ, gC);
    drawText(x, y, line, 0xFFC5AE36); y += step;
    std::snprintf(line, sizeof(line), "LINE %3d CLK %3d WSYNC %d", sLine, sClock, sFrameWsync);
    drawText(x, y, line, 0xFFE3E3E3); y += step;
    std::snprintf(line, sizeof(line), "TIA PF %02X BK %02X CTL %02X", tia.colupf, tia.colubk, tia.ctrlpf);
    drawText(x, y, line, 0xFFE3E3E3); y += step;
    std::snprintf(line, sizeof(line), "P0 %02X P1 %02X NUSIZ %02X %02X", tia.colup[0], tia.colup[1], tia.nusiz[0], tia.nusiz[1]);
    drawText(x, y, line, 0xFFE3E3E3); y += step;
    std::snprintf(line, sizeof(line), "ENAM %02X/%02X ENBL %02X VD%d%d%d", tia.enam[0], tia.enam[1], tia.enabl, tia.vdelp[0], tia.vdelp[1], tia.vdelbl);
    drawText(x, y, line, 0xFFE3E3E3); y += step;
    std::snprintf(line, sizeof(line), "POS P0 %3d P1 %3d B %3d", tia.pos[0], tia.pos[1], tia.bpos);
    drawText(x, y, line, 0xFFE3E3E3); y += step;
    std::snprintf(line, sizeof(line), "POS M0 %3d M1 %3d", tia.mpos[0], tia.mpos[1]);
    drawText(x, y, line, 0xFFE3E3E3); y += step;
    std::snprintf(line, sizeof(line), "SWCHA %02X SWCHB %02X I4 %02X", sSwcha, sSwchbBase, sInpt4);
    drawText(x, y, line, 0xFFE3E3E3); y += step;
    drawText(x, y, "--- RECENT EVENTS ---", 0xFF85CA3D); y += step;

    int shown = sLogCount < 8 ? sLogCount : 8;
    for (int i = shown - 1; i >= 0; i--) {
        int idx = (sLogHead - 1 - i + LOG_CAP) % LOG_CAP;
        drawText(x, y, sLog[idx].c_str(), 0xFFAAAAAA);
        y += step;
    }
}

void drawPauseBadge() {
    const char* msg = "PAUSED  P=RESUME";
    int w = textWidth(msg);
    int x = (VIS_WIDTH - w) / 2;
    int y = LOGICAL_HEIGHT - 26;
    blendRect(x - 4, y - 3, w + 8, 15, 0xFF000000, 190);
    drawRect(x - 4, y - 3, w + 8, 15, 0xFFDEC85E);
    drawText(x, y, msg, 0xFFDEC85E);
}

void composeDisplay() {
    int off;
    if (sVisibleStartLine >= 0) {
        off = sVisibleStartLine - 3;
    } else {
        off = sFrameLines - FRAME_HEIGHT + DISPLAY_TOP;
    }
    if (off < 0) off = 0;
    if (off > MAX_LINES - LOGICAL_HEIGHT) off = MAX_LINES - LOGICAL_HEIGHT;
    for (int y = 0; y < LOGICAL_HEIGHT; y++)
        std::memcpy(sDisplay[y], sFrame[y + off], VIS_WIDTH * sizeof(u32));
    if (sHelpVisible) drawHelpOverlay();
    if (sConsoleVisible) drawConsoleOverlay();
    if (sPaused) drawPauseBadge();
}

void dumpDisplayIfRequested() {
    static bool dumped = false;
    const char* path = std::getenv("VCS_DISPLAY_PPM");
    if (!path || dumped) return;
    int frame = 5;
    if (const char* e = std::getenv("VCS_DISPLAY_FRAME")) frame = std::atoi(e);
    if (sFrames != frame) return;
    FILE* fp = std::fopen(path, "wb");
    if (!fp) return;
    std::fprintf(fp, "P6\n%d %d\n255\n", VIS_WIDTH, LOGICAL_HEIGHT);
    for (int y = 0; y < LOGICAL_HEIGHT; y++) {
        for (int x = 0; x < VIS_WIDTH; x++) {
            u32 c = sDisplay[y][x];
            u8 rgb[3] = {(u8)(c >> 16), (u8)(c >> 8), (u8)c};
            std::fwrite(rgb, 1, 3, fp);
        }
    }
    std::fclose(fp);
    dumped = true;
    std::fprintf(stderr, "wrote display %s (frame %d)\n", path, sFrames);
}

void presentDisplay() {
    if (!sSdlReady) return;
    SDL_UpdateTexture(sTexture, nullptr, sDisplay, VIS_WIDTH * (int)sizeof(u32));
    SDL_RenderClear(sRenderer);
    SDL_RenderCopy(sRenderer, sTexture, nullptr, nullptr);
    SDL_RenderPresent(sRenderer);
}

void updateFps() {
    FrameClock::time_point now = FrameClock::now();
    if (sLastFrame.time_since_epoch().count() != 0) {
        double dt = std::chrono::duration<double>(now - sLastFrame).count();
        if (dt > 0.0) {
            double inst = 1.0 / dt;
            sFps = sFps > 0.0 ? sFps * 0.9 + inst * 0.1 : inst;
        }
    }
    sLastFrame = now;
}

void paceFrame() {
    if (!sPaceEnabled) return;
    FrameClock::time_point now = FrameClock::now();
    if (!sPaceInit) {
        sNextFrame = now;
        sPaceInit = true;
    } else {
        sNextFrame += std::chrono::microseconds(20000);
        if (sNextFrame < now) sNextFrame = now;
    }
    std::this_thread::sleep_until(sNextFrame);
}

void renderDisplay() {
    if (!sHeadless && sSdlReady) {
        paceFrame();
        updateFps();
    }
    composeDisplay();
    if (!sHeadless && sSdlReady) presentDisplay();
    dumpDisplayIfRequested();
}

void pauseLoop() {
    while (sPaused) {
        pollEvents();
        composeDisplay();
        presentDisplay();
        SDL_Delay(16);
    }
    sPaceInit = false;
    sLastFrame = FrameClock::time_point();
}

void presentFrame() {
    if (sFrames > 0 && sLine > 100 && sLine < MAX_LINES) sFrameLines = sLine;
    if (sLine < 100 || sLine >= MAX_LINES) sFrameLines = FRAME_HEIGHT;
    if (sHeadless) {
        if (sFrames < 8 || std::getenv("VCSDEBUG")) {
            std::fprintf(stderr,
                "frame %d: vblank=%02X vsync=%02X bkc=%02X pfc=%02X pf=%02X/%02X/%02X "
                "grp=%02X/%02X colup=%02X/%02X nusiz=%02X/%02X ctrl=%02X pos=%d/%d "
                "lines=%d pc=%04X\n",
                sFrames, tia.vblank, tia.vsync, tia.colubk, tia.colupf,
                tia.pf0, tia.pf1, tia.pf2, tia.grpDisp[0], tia.grpDisp[1],
                tia.colup[0], tia.colup[1], tia.nusiz[0], tia.nusiz[1],
                tia.ctrlpf, tia.pos[0], tia.pos[1], sFrameLines, gPC);
        }
        sFrames++;
        if (sHelpVisible || sConsoleVisible || std::getenv("VCS_DISPLAY_PPM")) {
            composeDisplay();
            dumpDisplayIfRequested();
        }
        if (sTargetFrames > 0 && sFrames >= sTargetFrames) {
            if (!sPpmPath.empty()) {
                FILE* fp = std::fopen(sPpmPath.c_str(), "wb");
                if (fp) {
                    std::fprintf(fp, "P6\n%d %d\n255\n", VIS_WIDTH, sFrameLines);
                    for (int y = 0; y < sFrameLines; y++) {
                        for (int x = 0; x < VIS_WIDTH; x++) {
                            u32 c = sFrame[y][x];
                            u8 rgb[3] = {(u8)(c >> 16), (u8)(c >> 8), (u8)c};
                            std::fwrite(rgb, 1, 3, fp);
                        }
                    }
                    std::fclose(fp);
                    std::fprintf(stderr, "wrote %s (%d frames)\n", sPpmPath.c_str(), sFrames);
                }
            } else {
                std::fprintf(stderr, "done: %d frames\n", sFrames);
            }
            std::exit(0);
        }
        return;
    }
    if (sSdlReady) {
        renderDisplay();
        sFrames++;
    }
}

void beginFrame() {
    if (std::getenv("VCS_FRAMESTATS") && sFrames > 0) {
        std::fprintf(stderr, "STAT f%d prevlines=%d wsync=%d timers=%d ticks=%llu\n",
                     sFrames, sLine, sFrameWsync, sFrameTimers,
                     (unsigned long long)(sTickCalls - sFrameTickStart));
    }
    if (std::getenv("VCS_RAMTRACE") && sFrames <= 80) {
        std::fprintf(stderr, "RT f%d room=%02X x=%02X y=%02X d=%02X st=%02X sprx=%02X sw=%02X mv=%02X e5=%02X e6=%02X de=%02X\n",
            sFrames, gRAM[0x0A], gRAM[0x0B], gRAM[0x0C], gRAM[0x0D], gRAM[0x0E],
            gRAM[0x06], gRAM[0x19], gRAM[0x1B], gRAM[0x65], gRAM[0x66], gRAM[0x5E]);
    }
    if (const char* e = std::getenv("VCS_RAMDUMP")) {
        int f = std::atoi(e);
        if (sFrames == f) {
            std::fprintf(stderr, "RAM f%d:", sFrames);
            for (int i = 0; i < 0x80; i++) std::fprintf(stderr, "%02X", gRAM[i]);
            std::fprintf(stderr, "\n");
        }
    }
    sFrameWsync = 0;
    sFrameTimers = 0;
    sFrameTickStart = sTickCalls;
    presentFrame();
    std::fill(&sFrame[0][0], &sFrame[0][0] + MAX_LINES * VIS_WIDTH, 0xFF000000u);
    sLine = 0;
}

void stepClock() {
    if (sClock >= VIS_START && sClock < VIS_START + VIS_WIDTH) {
        int x = sClock - VIS_START;
        if (sLine >= 0 && sLine < MAX_LINES) sFrame[sLine][x] = renderPixel(x);
    }
    sClock++;
    if (sClock >= LINE_CLOCKS) {
        sClock = 0;
        sLine++;
        if (sLine >= MAX_LINES) {
            sLine = 0;
            if (!sHadVsync) presentFrame();
        }
    }
}

void tiaWrite(u16 a, u8 v) {
    u8 r = a & 0x3F;
    if (r > 0x2C) return;
    if (sLogFrame >= 0 && sFrames >= sLogFrame) {
        std::fprintf(stderr, "TW f%d L%3d C%3d x%+4d r%02X v%02X pc%04X\n",
                     sFrames, sLine, sClock, sClock - VIS_START, r, v, gPC);
    }
    switch (r) {
        case 0x00:
            if ((v & 0x02) && !(tia.vsync & 0x02)) {
                sHadVsync = true;
                beginFrame();
            }
            tia.vsync = v;
            break;
        case 0x01:
            if ((tia.vblank & 0x02) && !(v & 0x02)) sVisibleStartLine = sLine;
            tia.vblank = v;
            break;
        case 0x02: sFrameWsync++; wsync(); break;
        case 0x03: rsync(); break;
        case 0x04: tia.nusiz[0] = v; break;
        case 0x05: tia.nusiz[1] = v; break;
        case 0x06: tia.colup[0] = v; break;
        case 0x07: tia.colup[1] = v; break;
        case 0x08: tia.colupf = v; break;
        case 0x09: tia.colubk = v; break;
        case 0x0A: tia.ctrlpf = v; break;
        case 0x0B: tia.refp[0] = v; break;
        case 0x0C: tia.refp[1] = v; break;
        case 0x0D: tia.pf0 = v; break;
        case 0x0E: tia.pf1 = v; break;
        case 0x0F: tia.pf2 = v; break;
        case 0x10:
            if (std::getenv("VCS_POSDBG")) std::fprintf(stderr, "RESP0 f%d L%d C%d pc=%04X\n", sFrames, sLine, sClock, gPC);
            tia.pos[0] = beamX() + 1; break;
        case 0x11:
            if (std::getenv("VCS_POSDBG")) std::fprintf(stderr, "RESP1 f%d L%d C%d pc=%04X\n", sFrames, sLine, sClock, gPC);
            tia.pos[1] = beamX() + 1; break;
        case 0x12: tia.mpos[0] = beamX() + 1; break;
        case 0x13: tia.mpos[1] = beamX() + 1; break;
        case 0x14: tia.bpos = beamX() + 1; break;
        case 0x15: tia.audc[0] = v; break;
        case 0x16: tia.audc[1] = v; break;
        case 0x17: tia.audf[0] = v; break;
        case 0x18: tia.audf[1] = v; break;
        case 0x19: tia.audv[0] = v; break;
        case 0x1A: tia.audv[1] = v; break;
        case 0x1B: grpWrite(0, v); break;
        case 0x1C: grpWrite(1, v); break;
        case 0x1D: tia.enam[0] = v; break;
        case 0x1E: tia.enam[1] = v; break;
        case 0x1F:
            tia.enablPrev = tia.enabl;
            tia.enabl = v;
            break;
        case 0x20: tia.hmp[0] = v; break;
        case 0x21: tia.hmp[1] = v; break;
        case 0x22: tia.hmm[0] = v; break;
        case 0x23: tia.hmm[1] = v; break;
        case 0x24: tia.hmbl = v; break;
        case 0x25: tia.vdelp[0] = (v & 0x02) != 0; break;
        case 0x26: tia.vdelp[1] = (v & 0x02) != 0; break;
        case 0x27: tia.vdelbl = (v & 0x02) != 0; break;
        case 0x28: tia.resmp[0] = v; break;
        case 0x29: tia.resmp[1] = v; break;
        case 0x2A:
            tia.off[0] = hmoveShift(tia.hmp[0]);
            tia.off[1] = hmoveShift(tia.hmp[1]);
            tia.moff[0] = hmoveShift(tia.hmm[0]);
            tia.moff[1] = hmoveShift(tia.hmm[1]);
            tia.boff = hmoveShift(tia.hmbl);
            break;
        case 0x2B:
            tia.hmp[0] = tia.hmp[1] = 0;
            tia.hmm[0] = tia.hmm[1] = 0;
            tia.hmbl = 0;
            break;
        case 0x2C:
            for (auto& c : tia.coll) c = 0;
            break;
        default: break;
    }
}

u8 tiaRead(u16 a) {
    u8 r = a & 0x0F;
    if (r < 8) {
        if (std::getenv("VCS_COLLDBG")) {
            std::fprintf(stderr, "CR f%d L%d x%3d r%02X v%02X pc%04X p%d/%d m%d/%d b%d "
                "offp%d/%d offm%d/%d offb%d pf%02X%02X%02X nus%02X%02X g%02X%02X e%d%d enab%d vd%d ctrl%02X\n",
                sFrames, sLine, sClock, r, tia.coll[r], gPC, tia.pos[0], tia.pos[1], tia.mpos[0], tia.mpos[1], tia.bpos,
                tia.off[0], tia.off[1], tia.moff[0], tia.moff[1], tia.boff,
                tia.pf0, tia.pf1, tia.pf2, tia.nusiz[0], tia.nusiz[1],
                tia.grpDisp[0], tia.grpDisp[1], tia.enam[0], tia.enam[1],
                tia.enabl, tia.vdelbl, tia.ctrlpf);
        }
        return tia.coll[r];
    }
    switch (r) {
        case 0x08: case 0x09: case 0x0A: case 0x0B: return 0x00;
        case 0x0C: return sInpt4;
        case 0x0D: return sInpt5;
        default: return 0;
    }
}

// ---------------------------------------------------------------------------
// RIOT (6532)
// ---------------------------------------------------------------------------
struct RiotState {
    u8 swacnt = 0;
    int timer = 0;
    int interval = 1;
    int accum = 0;
    bool intflag = false;
};

RiotState riot;

void riotCycle() {
    if (++riot.accum >= riot.interval) {
        riot.accum = 0;
        if (riot.timer <= 0) {
            riot.timer = 0xFF;
            riot.interval = 1;
            riot.intflag = true;
        } else {
            riot.timer--;
        }
    }
}

u8 riotRead(u16 a) {
    switch (a & 0x1F) {
        case 0x00: return sSwcha;
        case 0x01: return riot.swacnt;
        case 0x02: {
            u8 b = sSwchbBase;
            if (sResetFrame >= 0 && sFrames >= sResetFrame && sFrames < sResetFrame + 2) b &= ~0x01;
            if (sAutoStartFrame >= 0 && sFrames >= sAutoStartFrame && sFrames < sAutoStartFrame + 2) b &= ~0x01;
            if (sSelectFrame >= 0 && sFrames >= sSelectFrame && sFrames < sSelectFrame + 2) b &= ~0x02;
            return b;
        }
        case 0x04: return (u8)riot.timer;
        case 0x05: {
            u8 v = riot.intflag ? 0x80 : 0x00;
            riot.intflag = false;
            return v;
        }
        default: return 0;
    }
}

void riotWrite(u16 a, u8 v) {
    switch (a & 0x1F) {
        case 0x00: break; // SWCHA writes: port A data (unused)
        case 0x01: riot.swacnt = v; break;
        case 0x02: break; // SWCHB write: not writable
        case 0x14: sFrameTimers++; riot.timer = v; riot.interval = 1; riot.accum = 0; break;
        case 0x15: sFrameTimers++; riot.timer = v; riot.interval = 8; riot.accum = 0; break;
        case 0x16: sFrameTimers++; riot.timer = v; riot.interval = 64; riot.accum = 0; break;
        case 0x17: sFrameTimers++; riot.timer = v; riot.interval = 1024; riot.accum = 0; break;
        default: break;
    }
}

// ---------------------------------------------------------------------------
// Audio
// ---------------------------------------------------------------------------
u16 sPoly4 = 0xF, sPoly5 = 0x1F, sPoly9 = 0x1FF;
struct ChanState { int div = 0; u8 out = 0; };
ChanState sChan[2];
double sAudioAccum = 0.0;

void audioStep() {
    {
        u16 b = ((sPoly4 >> 3) ^ (sPoly4 >> 2)) & 1;
        sPoly4 = ((sPoly4 << 1) | b) & 0xF;
    }
    {
        u16 b = ((sPoly5 >> 4) ^ (sPoly5 >> 2)) & 1;
        sPoly5 = ((sPoly5 << 1) | b) & 0x1F;
    }
    {
        u16 b = ((sPoly9 >> 8) ^ (sPoly9 >> 4)) & 1;
        sPoly9 = ((sPoly9 << 1) | b) & 0x1FF;
    }
    for (int ch = 0; ch < 2; ch++) {
        if (--sChan[ch].div < 0) {
            sChan[ch].div = tia.audf[ch];
            u8 mode = tia.audc[ch] & 0x0F;
            u8 out;
            if (mode == 0) out = 1;
            else if (mode < 4) out = (sPoly4 >> 3) & 1;
            else if (mode < 8) out = (sPoly5 >> 4) & 1;
            else if (mode < 12) out = (sPoly9 >> 8) & 1;
            else out = sChan[ch].out ^ 1;
            sChan[ch].out = out;
        }
    }
}

void audioCallback(void*, Uint8* stream, int len) {
    Sint16* out = reinterpret_cast<Sint16*>(stream);
    int samples = len / (int)sizeof(Sint16);
    const double audioRate = 31113.0;
    const double sampleRate = 44100.0;
    for (int i = 0; i < samples; i++) {
        sAudioAccum += audioRate / sampleRate;
        while (sAudioAccum >= 1.0) {
            sAudioAccum -= 1.0;
            audioStep();
        }
        int mix = 0;
        for (int ch = 0; ch < 2; ch++) {
            if (!tia.audv[ch]) continue;
            int vol = tia.audv[ch];
            mix += (sChan[ch].out ? vol : -vol) * 90;
        }
        if (mix > 32767) mix = 32767;
        if (mix < -32768) mix = -32768;
        out[i] = (Sint16)mix;
    }
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------
void pollEvents() {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) sQuit = true;
        if (ev.type != SDL_KEYDOWN) continue;
        switch (ev.key.keysym.sym) {
            case SDLK_ESCAPE:
                if (sHelpVisible || sConsoleVisible) {
                    sHelpVisible = false;
                    sConsoleVisible = false;
                    logEvent("overlays closed");
                } else {
                    sQuit = true;
                }
                break;
            case SDLK_F1:
                sHelpVisible = !sHelpVisible;
                sConsoleVisible = false;
                logEvent("help %s", sHelpVisible ? "on" : "off");
                break;
            case SDLK_F2:
            case SDLK_BACKQUOTE:
                sConsoleVisible = !sConsoleVisible;
                sHelpVisible = false;
                logEvent("console %s", sConsoleVisible ? "on" : "off");
                break;
            case SDLK_p:
                sPaused = !sPaused;
                sPaceInit = false;
                if (sAudioDev) SDL_PauseAudioDevice(sAudioDev, sPaused ? 1 : 0);
                logEvent("pause %s", sPaused ? "on" : "off");
                break;
            default:
                break;
        }
    }
    if (sQuit) {
        std::fprintf(stderr, "quit\n");
        std::exit(0);
    }
    if (!sSdlReady) return;

    const Uint8* k = SDL_GetKeyboardState(nullptr);
    u8 sw = 0xFF;
    bool right = k[SDL_SCANCODE_RIGHT] || k[SDL_SCANCODE_D];
    bool left = k[SDL_SCANCODE_LEFT] || k[SDL_SCANCODE_A];
    bool down = k[SDL_SCANCODE_DOWN] || k[SDL_SCANCODE_S];
    bool up = k[SDL_SCANCODE_UP] || k[SDL_SCANCODE_W];
    if (right) sw &= ~0x80;
    if (left) sw &= ~0x40;
    if (down) sw &= ~0x20;
    if (up) sw &= ~0x10;
    sSwcha = sw;

    bool fire = k[SDL_SCANCODE_SPACE] || k[SDL_SCANCODE_Z];
    sInpt4 = fire ? 0x00 : 0x80;
    bool sel = k[SDL_SCANCODE_TAB];
    bool rst = k[SDL_SCANCODE_BACKSPACE] || k[SDL_SCANCODE_RETURN] || k[SDL_SCANCODE_KP_ENTER];
    if (rst) sAutoStartFrame = -1;
    bool color = true;
    u8 b = SWCHB_DEFAULT;
    if (sel) b &= ~0x02;
    if (rst) b &= ~0x01;
    if (!color) b &= ~0x08;
    sSwchbBase = b;

    int mask = 0;
    if (right) mask |= 0x01;
    if (left) mask |= 0x02;
    if (down) mask |= 0x04;
    if (up) mask |= 0x08;
    if (fire) mask |= 0x10;
    if (sel) mask |= 0x20;
    if (rst) mask |= 0x40;
    if (mask != sInputMask) {
        sInputMask = mask;
        if (mask == 0) {
            logEvent("input none");
        } else {
            std::string names;
            auto add = [&](bool on, const char* name) {
                if (!on) return;
                if (!names.empty()) names += ",";
                names += name;
            };
            add(right, "right");
            add(left, "left");
            add(down, "down");
            add(up, "up");
            add(fire, "fire");
            add(sel, "select");
            add(rst, "reset");
            logEvent("input %s", names.c_str());
        }
    }
}

// ---------------------------------------------------------------------------
// SDL setup
// ---------------------------------------------------------------------------
void sdlInit(int scale) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return;
    }
    sWindow = SDL_CreateWindow("VCSRelink - Adventure (PAL)", SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED, VIS_WIDTH * scale, LOGICAL_HEIGHT * scale,
                               SDL_WINDOW_RESIZABLE);
    if (!sWindow) {
        std::fprintf(stderr, "SDL window: %s\n", SDL_GetError());
        return;
    }
    if (!std::getenv("VCS_NOVSYNC")) {
        sRenderer = SDL_CreateRenderer(sWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!sRenderer)
            sRenderer = SDL_CreateRenderer(sWindow, -1, SDL_RENDERER_PRESENTVSYNC);
    }
    if (!sRenderer) sRenderer = SDL_CreateRenderer(sWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!sRenderer) sRenderer = SDL_CreateRenderer(sWindow, -1, 0);
    if (!sRenderer) {
        std::fprintf(stderr, "SDL renderer: %s\n", SDL_GetError());
        return;
    }
    SDL_RenderSetLogicalSize(sRenderer, VIS_WIDTH, LOGICAL_HEIGHT);
    sTexture = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ARGB8888,
                                 SDL_TEXTUREACCESS_STREAMING, VIS_WIDTH, LOGICAL_HEIGHT);
    if (!sTexture) {
        std::fprintf(stderr, "SDL texture: %s\n", SDL_GetError());
        return;
    }
    SDL_AudioSpec want;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 512;
    want.callback = audioCallback;
    sAudioDev = SDL_OpenAudioDevice(nullptr, 0, &want, nullptr, 0);
    if (sAudioDev) SDL_PauseAudioDevice(sAudioDev, 0);
    sSdlReady = true;
}

} // namespace

// ---------------------------------------------------------------------------
// Public CPU-side interface
// ---------------------------------------------------------------------------
void adc(u8 v) {
    unsigned s = (unsigned)gA + v + gC;
    u8 bin = (u8)s;
    gV = (((~(gA ^ v)) & (gA ^ bin) & 0x80) != 0);
    gZ = (bin == 0);
    gN = (bin >> 7);
    if (!gD) {
        gA = bin;
        gC = (s > 0xFF);
    } else {
        unsigned lo = (gA & 0x0F) + (v & 0x0F) + gC;
        unsigned hi = (gA >> 4) + (v >> 4);
        if (lo > 9) { lo += 6; hi++; }
        if (hi > 9) hi += 6;
        gC = (hi > 0x0F);
        gA = (u8)((hi << 4) | (lo & 0x0F));
    }
}

void sbc(u8 v) {
    unsigned d = (unsigned)gA - v - (1 - gC);
    u8 bin = (u8)d;
    gV = (((gA ^ v) & (gA ^ bin) & 0x80) != 0);
    gZ = (bin == 0);
    gN = (bin >> 7);
    if (!gD) {
        gA = bin;
        gC = (d < 0x100);
    } else {
        int lo = (gA & 0x0F) - (v & 0x0F) - (1 - gC);
        int hi = (gA >> 4) - (v >> 4);
        if (lo < 0) { lo -= 6; hi--; }
        if (hi < 0) hi -= 6;
        gC = (hi >= 0);
        gA = (u8)(((hi << 4) | (lo & 0x0F)) & 0xFF);
    }
}

u8 rd(u16 a) {
    a &= 0x1FFF;
    u8 retr;
    if (a & 0x1000) retr = gRomBanksPtr[gBank][a & 0xFFF];
    else {
        switch (a & 0x1280) {
            case 0x0000:
            case 0x0200: retr = tiaRead(a); break;
            case 0x0080: retr = gRAM[a & 0x7F]; break;
            case 0x0280: retr = riotRead(a); break;
            default: retr = 0; break;
        }
    }
    if (std::getenv("VCS_RDTRACE")) {
        std::fprintf(stderr, "RD %04X=%02X pc=%04X c=%llu\n", a, retr, gPC, (unsigned long long)sCycles);
    }
    if (std::getenv("VCS_IODBG") && (a == 0x280 || a == 0x282)) {
        std::fprintf(stderr, "IO f%d L%d pc=%04X %04X=%02X\n", sFrames, sLine, gPC, a, (a==0x280?sSwcha:sSwchbBase));
    }
    return retr;
}

void wr(u16 a, u8 v) {
    a &= 0x1FFF;
    if (std::getenv("VCS_WRTRACE")) {
        std::fprintf(stderr, "WR %04X=%02X pc=%04X c=%llu\n", a, v, gPC, (unsigned long long)sCycles);
    }
    if (a & 0x1000) {
        if (gRomBanks == 2) {
            u16 off = a & 0xFFF;
            if ((off & 0xFFE) == 0xFF8) gBank = (off & 1) ? 1 : 0;
        }
        return;
    }
    switch (a & 0x1280) {
        case 0x0000:
        case 0x0200: tiaWrite(a, v); return;
        case 0x0080: gRAM[a & 0x7F] = v; return;
        case 0x0280: riotWrite(a, v); return;
        default: return;
    }
}

void tick(int cycles) {
    if (sPaused && !sHeadless) pauseLoop();
    if (std::getenv("VCS_REGTRACE")) {
        std::fprintf(stderr, "R%8llu L%3d %04X %02X %02X %02X %02X %d%d%d%d%d%d\n",
                     (unsigned long long)sTickCalls, sLine, gPC, gA, gX, gY, gS,
                     gC, gZ, gN, gV, gI, gD);
    }
    if (sTraceFrame >= 0 && sFrames == sTraceFrame) {
        std::fprintf(stderr, "T %3d %04X\n", sLine, gPC);
    } else if (sTraceAll && sTickCalls < sTraceAllLimit) {
        std::fprintf(stderr, "TA %8llu L%3d %04X\n",
                     (unsigned long long)sTickCalls, sLine, gPC);
    }
    if ((++sTickCalls & 0x1FF) == 0) {
        if (!sHeadless) pollEvents();
        else if (sTargetFrames > 0 && !sHadVsync && sCycles > 20000000ull) {
            std::fprintf(stderr, "no VSYNC seen, dumping\n");
            sTargetFrames = -1;
            sPpmPath.clear();
            sFrames = 0;
            presentFrame();
            std::exit(0);
        }
    }
    for (int i = 0; i < cycles; i++) {
        riotCycle();
        for (int c = 0; c < 3; c++) stepClock();
        sCycles++;
    }
}

void wsync() {
    while (sClock != 0) stepClock();
}

void rsync() {
    sClock = 0;
}

void push8(u8 v) {
    gRAM[gS & 0x7F] = v;
    gS--;
}

u8 pull8() {
    gS++;
    return gRAM[gS & 0x7F];
}

void push16(u16 v) {
    push8((u8)(v >> 8));
    push8((u8)v);
}

u16 pull16() {
    u8 lo = pull8();
    u8 hi = pull8();
    return (u16)(lo | (hi << 8));
}

void fatal(const char* msg, u16 pc) {
    std::fprintf(stderr, "fatal: %s at PC=%04X bank=%d\n", msg, pc, gBank);
    std::exit(2);
}

void init(int argc, char** argv) {
    sArgc = argc;
    sArgv = argv;
    int scale = 2;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--headless") sHeadless = true;
        else if (a == "--frames" && i + 1 < argc) sTargetFrames = std::atol(argv[++i]);
        else if (a == "--ppm" && i + 1 < argc) sPpmPath = argv[++i];
        else if (a == "--scale" && i + 1 < argc) scale = std::atoi(argv[++i]);
        else if (a == "--nopace") sPaceEnabled = false;
    }
    if (scale < 1) scale = 1;
    if (std::getenv("VCS_NOPACE")) sPaceEnabled = false;
    if (std::getenv("VCS_HELP")) {
        sHelpVisible = true;
        sConsoleVisible = false;
    }
    if (std::getenv("VCS_CONSOLE")) {
        sConsoleVisible = true;
        sHelpVisible = false;
    }
    if (const char* e = std::getenv("VCS_INPUT")) {
        std::string in = e;
        size_t p = 0;
        while (p <= in.size()) {
            size_t q = in.find(',', p);
            std::string tok = in.substr(p, q == std::string::npos ? std::string::npos : q - p);
            if (tok == "right") sSwcha &= ~0x80;
            else if (tok == "left") sSwcha &= ~0x40;
            else if (tok == "down") sSwcha &= ~0x20;
            else if (tok == "up") sSwcha &= ~0x10;
            else if (tok == "fire") sInpt4 = 0x00;
            else if (tok == "select") sSwchbBase &= ~0x02;
            else if (tok == "reset") sSwchbBase &= ~0x01;
            if (q == std::string::npos) break;
            p = q + 1;
        }
    }
    if (const char* e = std::getenv("VCS_RESETFRAME")) sResetFrame = std::atoi(e);
    if (const char* e = std::getenv("VCS_AUTOSTART_FRAME")) sAutoStartFrame = std::atoi(e);
    if (sResetFrame >= 0 || std::getenv("VCS_NOAUTOSTART")) sAutoStartFrame = -1;
    if (const char* e = std::getenv("VCS_SELECTFRAME")) sSelectFrame = std::atoi(e);
    if (const char* e = std::getenv("VCS_LOG_FRAME")) sLogFrame = std::atoi(e);
    if (const char* e = std::getenv("VCS_TRACE_FRAME")) sTraceFrame = std::atoi(e);
    if (const char* e = std::getenv("VCS_TRACE_ALL")) {
        sTraceAll = true;
        sTraceAllLimit = (u64)std::strtoull(e, nullptr, 10);
        if (sTraceAllLimit == 0) sTraceAllLimit = 200000;
    }

    for (auto& row : sFrame)
        for (auto& px : row) px = 0xFF000000u;

    std::memset(gRAM, 0, sizeof(gRAM));
    gA = gX = gY = 0;
    gS = 0xFD;
    gC = gZ = gN = gV = 0;
    gI = 1;
    gD = 0;
    gBank = 0;

    gRomBanks = ROM_BANKS;
    gRomBanksPtr[0] = ROM_DATA;
    if (gRomBanks > 1) gRomBanksPtr[1] = ROM_DATA + 4096;

    logEvent("init b=%d pace=%d auto=%d", gRomBanks, sPaceEnabled ? 1 : 0, sAutoStartFrame);

    if (!sHeadless) sdlInit(scale);
    else sSdlReady = false;
}

} // namespace rt
