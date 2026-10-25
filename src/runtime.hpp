#pragma once
#include <cstdint>

namespace rt {
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

extern u8 gA, gX, gY, gS;
extern u8 gC, gZ, gN, gV, gI, gD;
extern u8 gBank;
extern u16 gPC;
extern int gRomBanks;
extern u8 gRAM[0x100];
extern const u8* gRomBanksPtr[2];
extern void* MAP[2][0x1000];

extern u8 ROM_DATA[];
extern int ROM_BANKS;

u8 rd(u16 a);
void wr(u16 a, u8 v);
void adc(u8 v);
void sbc(u8 v);
void tick(int cycles);
void wsync();
void rsync();
void push8(u8 v);
u8 pull8();
void push16(u16 v);
u16 pull16();
[[noreturn]] void fatal(const char* msg, u16 pc);
void init(int argc, char** argv);
} // namespace rt
