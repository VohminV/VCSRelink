// VCSRelink - static 6502 -> C++ recompiler for Atari 2600 ROMs.
// Usage: relinker <rom.bin> <out.cpp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

using std::string;
using std::vector;
typedef uint8_t u8;
typedef uint16_t u16;

enum Mode {
    IMP, ACC, IMM, ZP, ZPX, ZPY, ABS, ABX, ABY, IND, IZX, IZY, REL
};

struct Op {
    const char* nm;
    Mode mode;
    int cyc;
};

static Op gOps[256];

static void defOp(int code, const char* nm, Mode m, int cyc) {
    gOps[code] = {nm, m, cyc};
}

static void buildTable() {
    for (int i = 0; i < 256; i++) gOps[i] = {"NOP", IMP, 2};

    defOp(0x00, "BRK", IMP, 7);
    defOp(0x01, "ORA", IZX, 6);
    defOp(0x05, "ORA", ZP, 3);
    defOp(0x06, "ASL", ZP, 5);
    defOp(0x08, "PHP", IMP, 3);
    defOp(0x09, "ORA", IMM, 2);
    defOp(0x0A, "ASL", ACC, 2);
    defOp(0x0D, "ORA", ABS, 4);
    defOp(0x0E, "ASL", ABS, 6);
    defOp(0x10, "BPL", REL, 2);
    defOp(0x11, "ORA", IZY, 5);
    defOp(0x15, "ORA", ZPX, 4);
    defOp(0x16, "ASL", ZPX, 6);
    defOp(0x18, "CLC", IMP, 2);
    defOp(0x19, "ORA", ABY, 4);
    defOp(0x1D, "ORA", ABX, 4);
    defOp(0x1E, "ASL", ABX, 7);

    defOp(0x20, "JSR", ABS, 6);
    defOp(0x21, "AND", IZX, 6);
    defOp(0x24, "BIT", ZP, 3);
    defOp(0x25, "AND", ZP, 3);
    defOp(0x26, "ROL", ZP, 5);
    defOp(0x28, "PLP", IMP, 4);
    defOp(0x29, "AND", IMM, 2);
    defOp(0x2A, "ROL", ACC, 2);
    defOp(0x2C, "BIT", ABS, 4);
    defOp(0x2D, "AND", ABS, 4);
    defOp(0x2E, "ROL", ABS, 6);
    defOp(0x30, "BMI", REL, 2);
    defOp(0x31, "AND", IZY, 5);
    defOp(0x35, "AND", ZPX, 4);
    defOp(0x36, "ROL", ZPX, 6);
    defOp(0x38, "SEC", IMP, 2);
    defOp(0x39, "AND", ABY, 4);
    defOp(0x3D, "AND", ABX, 4);
    defOp(0x3E, "ROL", ABX, 7);

    defOp(0x40, "RTI", IMP, 6);
    defOp(0x41, "EOR", IZX, 6);
    defOp(0x45, "EOR", ZP, 3);
    defOp(0x46, "LSR", ZP, 5);
    defOp(0x48, "PHA", IMP, 3);
    defOp(0x49, "EOR", IMM, 2);
    defOp(0x4A, "LSR", ACC, 2);
    defOp(0x4C, "JMP", ABS, 3);
    defOp(0x4D, "EOR", ABS, 4);
    defOp(0x4E, "LSR", ABS, 6);
    defOp(0x50, "BVC", REL, 2);
    defOp(0x51, "EOR", IZY, 5);
    defOp(0x55, "EOR", ZPX, 4);
    defOp(0x56, "LSR", ZPX, 6);
    defOp(0x58, "CLI", IMP, 2);
    defOp(0x59, "EOR", ABY, 4);
    defOp(0x5D, "EOR", ABX, 4);
    defOp(0x5E, "LSR", ABX, 7);

    defOp(0x60, "RTS", IMP, 6);
    defOp(0x61, "ADC", IZX, 6);
    defOp(0x65, "ADC", ZP, 3);
    defOp(0x66, "ROR", ZP, 5);
    defOp(0x68, "PLA", IMP, 4);
    defOp(0x69, "ADC", IMM, 2);
    defOp(0x6A, "ROR", ACC, 2);
    defOp(0x6C, "JMP", IND, 5);
    defOp(0x6D, "ADC", ABS, 4);
    defOp(0x6E, "ROR", ABS, 6);
    defOp(0x70, "BVS", REL, 2);
    defOp(0x71, "ADC", IZY, 5);
    defOp(0x75, "ADC", ZPX, 4);
    defOp(0x76, "ROR", ZPX, 6);
    defOp(0x78, "SEI", IMP, 2);
    defOp(0x79, "ADC", ABY, 4);
    defOp(0x7D, "ADC", ABX, 4);
    defOp(0x7E, "ROR", ABX, 7);

    defOp(0x81, "STA", IZX, 6);
    defOp(0x84, "STY", ZP, 3);
    defOp(0x85, "STA", ZP, 3);
    defOp(0x86, "STX", ZP, 3);
    defOp(0x88, "DEY", IMP, 2);
    defOp(0x8A, "TXA", IMP, 2);
    defOp(0x8C, "STY", ABS, 4);
    defOp(0x8D, "STA", ABS, 4);
    defOp(0x8E, "STX", ABS, 4);
    defOp(0x90, "BCC", REL, 2);
    defOp(0x91, "STA", IZY, 6);
    defOp(0x94, "STY", ZPX, 4);
    defOp(0x95, "STA", ZPX, 4);
    defOp(0x96, "STX", ZPY, 4);
    defOp(0x98, "TYA", IMP, 2);
    defOp(0x99, "STA", ABY, 5);
    defOp(0x9A, "TXS", IMP, 2);
    defOp(0x9D, "STA", ABX, 5);

    defOp(0xA0, "LDY", IMM, 2);
    defOp(0xA1, "LDA", IZX, 6);
    defOp(0xA2, "LDX", IMM, 2);
    defOp(0xA4, "LDY", ZP, 3);
    defOp(0xA5, "LDA", ZP, 3);
    defOp(0xA6, "LDX", ZP, 3);
    defOp(0xA8, "TAY", IMP, 2);
    defOp(0xA9, "LDA", IMM, 2);
    defOp(0xAA, "TAX", IMP, 2);
    defOp(0xAC, "LDY", ABS, 4);
    defOp(0xAD, "LDA", ABS, 4);
    defOp(0xAE, "LDX", ABS, 4);
    defOp(0xB0, "BCS", REL, 2);
    defOp(0xB1, "LDA", IZY, 5);
    defOp(0xB4, "LDY", ZPX, 4);
    defOp(0xB5, "LDA", ZPX, 4);
    defOp(0xB6, "LDX", ZPY, 4);
    defOp(0xB8, "CLV", IMP, 2);
    defOp(0xB9, "LDA", ABY, 4);
    defOp(0xBA, "TSX", IMP, 2);
    defOp(0xBC, "LDY", ABX, 4);
    defOp(0xBD, "LDA", ABX, 4);
    defOp(0xBE, "LDX", ABY, 4);

    defOp(0xC0, "CPY", IMM, 2);
    defOp(0xC1, "CMP", IZX, 6);
    defOp(0xC4, "CPY", ZP, 3);
    defOp(0xC5, "CMP", ZP, 3);
    defOp(0xC6, "DEC", ZP, 5);
    defOp(0xC8, "INY", IMP, 2);
    defOp(0xC9, "CMP", IMM, 2);
    defOp(0xCA, "DEX", IMP, 2);
    defOp(0xCC, "CPY", ABS, 4);
    defOp(0xCD, "CMP", ABS, 4);
    defOp(0xCE, "DEC", ABS, 6);
    defOp(0xD0, "BNE", REL, 2);
    defOp(0xD1, "CMP", IZY, 5);
    defOp(0xD5, "CMP", ZPX, 4);
    defOp(0xD6, "DEC", ZPX, 6);
    defOp(0xD8, "CLD", IMP, 2);
    defOp(0xD9, "CMP", ABY, 4);
    defOp(0xDD, "CMP", ABX, 4);
    defOp(0xDE, "DEC", ABX, 7);

    defOp(0xE0, "CPX", IMM, 2);
    defOp(0xE1, "SBC", IZX, 6);
    defOp(0xE4, "CPX", ZP, 3);
    defOp(0xE5, "SBC", ZP, 3);
    defOp(0xE6, "INC", ZP, 5);
    defOp(0xE8, "INX", IMP, 2);
    defOp(0xE9, "SBC", IMM, 2);
    defOp(0xEA, "NOP", IMP, 2);
    defOp(0xEC, "CPX", ABS, 4);
    defOp(0xED, "SBC", ABS, 4);
    defOp(0xEE, "INC", ABS, 6);
    defOp(0xF0, "BEQ", REL, 2);
    defOp(0xF1, "SBC", IZY, 5);
    defOp(0xF5, "SBC", ZPX, 4);
    defOp(0xF6, "INC", ZPX, 6);
    defOp(0xF8, "SED", IMP, 2);
    defOp(0xF9, "SBC", ABY, 4);
    defOp(0xFD, "SBC", ABX, 4);
    defOp(0xFE, "INC", ABX, 7);

    // Common undocumented opcodes used by homebrew / later games.
    defOp(0x1A, "NOP", IMP, 2);
    defOp(0x3A, "NOP", IMP, 2);
    defOp(0x5A, "NOP", IMP, 2);
    defOp(0x7A, "NOP", IMP, 2);
    defOp(0xDA, "NOP", IMP, 2);
    defOp(0xFA, "NOP", IMP, 2);
    defOp(0x80, "NOP", IMM, 2);
    defOp(0x82, "NOP", IMM, 2);
    defOp(0x89, "NOP", IMM, 2);
    defOp(0xC2, "NOP", IMM, 2);
    defOp(0xE2, "NOP", IMM, 2);
    defOp(0x04, "NOP", ZP, 3);
    defOp(0x44, "NOP", ZP, 3);
    defOp(0x64, "NOP", ZP, 3);
    defOp(0x14, "NOP", ZPX, 4);
    defOp(0x34, "NOP", ZPX, 4);
    defOp(0x54, "NOP", ZPX, 4);
    defOp(0x74, "NOP", ZPX, 4);
    defOp(0xD4, "NOP", ZPX, 4);
    defOp(0xF4, "NOP", ZPX, 4);
    defOp(0x0C, "NOP", ABS, 4);
    defOp(0x1C, "NOP", ABX, 4);
    defOp(0x3C, "NOP", ABX, 4);
    defOp(0x5C, "NOP", ABX, 4);
    defOp(0x7C, "NOP", ABX, 4);
    defOp(0xDC, "NOP", ABX, 4);
    defOp(0xFC, "NOP", ABX, 4);
    defOp(0xEB, "SBC", IMM, 2);
    defOp(0xA7, "LAX", ZP, 3);
    defOp(0xB7, "LAX", ZPY, 4);
    defOp(0xAF, "LAX", ABS, 4);
    defOp(0xBF, "LAX", ABY, 4);
    defOp(0xA3, "LAX", IZX, 6);
    defOp(0xB3, "LAX", IZY, 5);
    defOp(0xAB, "LAX", IMM, 2);
    defOp(0x87, "SAX", ZP, 3);
    defOp(0x97, "SAX", ZPY, 4);
    defOp(0x8F, "SAX", ABS, 4);
    defOp(0x83, "SAX", IZX, 6);

    // Remaining illegals: keep correct instruction length, behave as NOP.
    defOp(0xC7, "DCP", ZP, 5);   defOp(0xD7, "DCP", ZPX, 6);
    defOp(0xCF, "DCP", ABS, 6);  defOp(0xDF, "DCP", ABX, 7);
    defOp(0xDB, "DCP", ABY, 7);  defOp(0xC3, "DCP", IZX, 8);
    defOp(0xD3, "DCP", IZY, 8);
    defOp(0xE7, "ISC", ZP, 5);   defOp(0xF7, "ISC", ZPX, 6);
    defOp(0xEF, "ISC", ABS, 6);  defOp(0xFF, "ISC", ABX, 7);
    defOp(0xFB, "ISC", ABY, 7);  defOp(0xE3, "ISC", IZX, 8);
    defOp(0xF3, "ISC", IZY, 8);
    defOp(0x07, "SLO", ZP, 5);   defOp(0x17, "SLO", ZPX, 6);
    defOp(0x0F, "SLO", ABS, 6);  defOp(0x1F, "SLO", ABX, 7);
    defOp(0x1B, "SLO", ABY, 7);  defOp(0x03, "SLO", IZX, 8);
    defOp(0x13, "SLO", IZY, 8);
    defOp(0x27, "RLA", ZP, 5);   defOp(0x37, "RLA", ZPX, 6);
    defOp(0x2F, "RLA", ABS, 6);  defOp(0x3F, "RLA", ABX, 7);
    defOp(0x3B, "RLA", ABY, 7);  defOp(0x23, "RLA", IZX, 8);
    defOp(0x33, "RLA", IZY, 8);
    defOp(0x47, "SRE", ZP, 5);   defOp(0x57, "SRE", ZPX, 6);
    defOp(0x4F, "SRE", ABS, 6);  defOp(0x5F, "SRE", ABX, 7);
    defOp(0x5B, "SRE", ABY, 7);  defOp(0x43, "SRE", IZX, 8);
    defOp(0x53, "SRE", IZY, 8);
    defOp(0x67, "RRA", ZP, 5);   defOp(0x77, "RRA", ZPX, 6);
    defOp(0x6F, "RRA", ABS, 6);  defOp(0x7F, "RRA", ABX, 7);
    defOp(0x7B, "RRA", ABY, 7);  defOp(0x63, "RRA", IZX, 8);
    defOp(0x73, "RRA", IZY, 8);
}

static int lenOf(Mode m) {
    switch (m) {
        case IMP: case ACC: return 1;
        case ABS: case ABX: case ABY: case IND: return 3;
        default: return 2;
    }
}

static string hex2(int v) {
    char b[8];
    std::snprintf(b, sizeof b, "%02X", v & 0xFF);
    return b;
}

static string hex4(int v) {
    char b[8];
    std::snprintf(b, sizeof b, "%04X", v & 0xFFFF);
    return b;
}

static string label(int bank, int addr) {
    char b[32];
    std::snprintf(b, sizeof b, "L%d_%03X", bank, addr & 0xFFF);
    return b;
}

struct Addr {
    string pre;
    string expr;
    string pageCross;
    bool ram = false;
    bool literal = false;
    string ramExpr;
};

class Emitter {
public:
    vector<u8> rom;
    int banks = 1;
    int romSize = 0;
    std::ofstream out;

    int byteAt(int bank, int addr) {
        int off = bank * 4096 + (addr - 0x1000);
        if (addr < 0x1000 || addr > 0x1FFF) return 0;
        if (off < 0 || off >= romSize) return 0;
        return rom[off];
    }

    Addr resolve(Mode m, int b1, int b2) {
        Addr a;
        switch (m) {
            case IMM:
                a.expr = "0x" + hex2(b1);
                a.literal = true;
                break;
            case ZP:
                a.expr = "0x00" + hex2(b1) + "u";
                break;
            case ZPX:
                a.pre = "u16 ea = (u8)(0x" + hex2(b1) + " + gX);\n";
                a.expr = "ea";
                break;
            case ZPY:
                a.pre = "u16 ea = (u8)(0x" + hex2(b1) + " + gY);\n";
                a.expr = "ea";
                break;
            case ABS:
                a.expr = "0x" + hex4(b1 | (b2 << 8));
                break;
            case ABX:
                a.pre = "u16 ea = (u16)(0x" + hex4(b1 | (b2 << 8)) + " + gX);\n";
                a.pageCross = "((0x" + hex4(b1 | (b2 << 8)) + " ^ ea) & 0xFF00) != 0";
                a.expr = "ea";
                break;
            case ABY:
                a.pre = "u16 ea = (u16)(0x" + hex4(b1 | (b2 << 8)) + " + gY);\n";
                a.pageCross = "((0x" + hex4(b1 | (b2 << 8)) + " ^ ea) & 0xFF00) != 0";
                a.expr = "ea";
                break;
            case IZX:
                a.pre = "u8 zp = (u8)(0x" + hex2(b1) + " + gX);\n";
                a.pre += "u16 ea = (u16)(rd(zp) | (rd((u8)(zp + 1)) << 8));\n";
                a.expr = "ea";
                break;
            case IZY:
                a.pre = "u16 base = (u16)(rd(0x" + hex2(b1) + ") | (rd(0x" +
                        hex2((b1 + 1) & 0xFF) + ") << 8));\n";
                a.pre += "u16 ea = (u16)(base + gY);\n";
                a.pageCross = "((base ^ ea) & 0xFF00) != 0";
                a.expr = "ea";
                break;
            default:
                break;
        }
        return a;
    }

    string readExpr(const Addr& a) {
        if (a.ram) return a.ramExpr;
        if (a.literal) return a.expr;
        return "rd(" + a.expr + ")";
    }

    string writeStmt(const Addr& a, const string& val) {
        if (a.ram) return a.ramExpr + " = " + val + ";";
        return "wr(" + a.expr + ", " + val + ");";
    }

    void emitInstruction(int bank, int pc) {
        u8 opcode = (u8)byteAt(bank, pc);
        const Op& op = gOps[opcode];
        int len = lenOf(op.mode);
        int b1 = byteAt(bank, pc + 1);
        int b2 = byteAt(bank, pc + 2);

        string nxt;
        int nextAddr = pc + len;
        if (nextAddr <= 0x1FFF) nxt = label(bank, nextAddr);
        else nxt = "L_trap";

        Addr addr = resolve(op.mode, b1, b2);
        string nm = op.nm;
        int cyc = op.cyc;

        // Debug comment
        out << label(bank, pc) << ": { // " << hex4(pc) << ": " << nm;
        if (op.mode == IMM) out << " #$" << hex2(b1);
        else if (op.mode == ZP) out << " $" << hex2(b1);
        else if (op.mode == ZPX) out << " $" << hex2(b1) << ",X";
        else if (op.mode == ZPY) out << " $" << hex2(b1) << ",Y";
        else if (op.mode == ABS) out << " $" << hex4(b1 | (b2 << 8));
        else if (op.mode == ABX) out << " $" << hex4(b1 | (b2 << 8)) << ",X";
        else if (op.mode == ABY) out << " $" << hex4(b1 | (b2 << 8)) << ",Y";
        else if (op.mode == IND) out << " ($" << hex4(b1 | (b2 << 8)) << ")";
        else if (op.mode == IZX) out << " ($" << hex2(b1) << ",X)";
        else if (op.mode == IZY) out << " ($" << hex2(b1) << "),Y";
        else if (op.mode == REL) {
            int t = pc + 2 + (int8_t)b1;
            out << " $" << hex4(t);
        } else if (op.mode == ACC) out << " A";
        out << "\n";
        out << "  gPC = 0x" << hex4(pc) << ";\n";

        auto pre = [&]() {
            if (!addr.pre.empty()) out << "  " << addr.pre;
        };
        auto penalty = [&]() {
            if (!addr.pageCross.empty())
                out << "  if (" << addr.pageCross << ") tick(1);\n";
        };

        // Loads
        if (nm == "LDA" || nm == "LDX" || nm == "LDY" || nm == "LAX") {
            const char* reg = nm == "LDX" ? "gX" : (nm == "LDY" ? "gY" : "gA");
            out << "  tick(" << cyc << ");\n";
            pre();
            penalty();
            out << "  u8 v = " << readExpr(addr) << ";\n";
            out << "  " << reg << " = v;\n";
            if (nm == "LAX") out << "  gX = v;\n";
            out << "  gZ = (v == 0); gN = (v >> 7);\n";
            out << "  goto " << nxt << ";\n";
            out << "}\n";
            return;
        }

        if (nm == "STA" || nm == "STX" || nm == "STY" || nm == "SAX") {
            const char* reg = nm == "STX" ? "gX" : (nm == "STY" ? "gY" : "gA");
            out << "  tick(" << cyc << ");\n";
            pre();
            if (nm == "SAX") {
                out << "  u8 v = (u8)(gA & gX);\n";
                out << "  " << writeStmt(addr, "v") << "\n";
            } else {
                out << "  " << writeStmt(addr, reg) << "\n";
            }
            out << "  goto " << nxt << ";\n";
            out << "}\n";
            return;
        }

        if (nm == "ADC" || nm == "SBC") {
            out << "  tick(" << cyc << ");\n";
            pre();
            penalty();
            out << "  u8 v = " << readExpr(addr) << ";\n";
            out << "  " << (nm == "ADC" ? "adc" : "sbc") << "(v);\n";
            out << "  goto " << nxt << ";\n}\n";
            return;
        }

        if (nm == "AND" || nm == "ORA" || nm == "EOR") {
            out << "  tick(" << cyc << ");\n";
            pre();
            penalty();
            out << "  u8 v = " << readExpr(addr) << ";\n";
            if (nm == "AND") out << "  gA = (u8)(gA & v);\n";
            else if (nm == "ORA") out << "  gA = (u8)(gA | v);\n";
            else out << "  gA = (u8)(gA ^ v);\n";
            out << "  gZ = (gA == 0); gN = (gA >> 7);\n";
            out << "  goto " << nxt << ";\n}\n";
            return;
        }

        if (nm == "CMP" || nm == "CPX" || nm == "CPY") {
            const char* reg = nm == "CPX" ? "gX" : (nm == "CPY" ? "gY" : "gA");
            out << "  tick(" << cyc << ");\n";
            pre();
            penalty();
            out << "  u8 v = " << readExpr(addr) << ";\n";
            out << "  unsigned s = (unsigned)" << reg << " - v;\n";
            out << "  gC = (" << reg << " >= v); gZ = (" << reg << " == v);\n";
            out << "  gN = ((u8)s >> 7);\n";
            out << "  goto " << nxt << ";\n}\n";
            return;
        }

        if (nm == "BIT") {
            out << "  tick(" << cyc << ");\n";
            pre();
            out << "  u8 v = " << readExpr(addr) << ";\n";
            out << "  gZ = ((gA & v) == 0); gN = (v >> 7); gV = ((v >> 6) & 1);\n";
            out << "  goto " << nxt << ";\n}\n";
            return;
        }

        if (nm == "ASL" || nm == "LSR" || nm == "ROL" || nm == "ROR") {
            out << "  tick(" << cyc << ");\n";
            if (op.mode == ACC) {
                if (nm == "ASL") {
                    out << "  gC = (gA >> 7); gA = (u8)(gA << 1);\n";
                } else if (nm == "LSR") {
                    out << "  gC = (gA & 1); gA >>= 1;\n";
                } else if (nm == "ROL") {
                    out << "  u8 c = gC; gC = (gA >> 7); gA = (u8)((gA << 1) | c);\n";
                } else {
                    out << "  u8 c = gC; gC = (gA & 1); gA = (u8)((gA >> 1) | (c << 7));\n";
                }
                out << "  gZ = (gA == 0); gN = (gA >> 7);\n";
            } else {
                pre();
                out << "  u8 v = " << readExpr(addr) << ";\n";
                if (nm == "ASL") {
                    out << "  gC = (v >> 7); v = (u8)(v << 1);\n";
                } else if (nm == "LSR") {
                    out << "  gC = (v & 1); v >>= 1;\n";
                } else if (nm == "ROL") {
                    out << "  u8 c = gC; gC = (v >> 7); v = (u8)((v << 1) | c);\n";
                } else {
                    out << "  u8 c = gC; gC = (v & 1); v = (u8)((v >> 1) | (c << 7));\n";
                }
                out << "  gZ = (v == 0); gN = (v >> 7);\n";
                out << "  " << writeStmt(addr, "v") << "\n";
            }
            out << "  goto " << nxt << ";\n}\n";
            return;
        }

        if (nm == "INC" || nm == "DEC") {
            out << "  tick(" << cyc << ");\n";
            pre();
            out << "  u8 v = " << readExpr(addr) << ";\n";
            if (nm == "INC") out << "  v++;\n";
            else out << "  v--;\n";
            out << "  gZ = (v == 0); gN = (v >> 7);\n";
            out << "  " << writeStmt(addr, "v") << "\n";
            out << "  goto " << nxt << ";\n}\n";
            return;
        }

        if (nm == "BPL" || nm == "BMI" || nm == "BVC" || nm == "BVS" ||
            nm == "BCC" || nm == "BCS" || nm == "BNE" || nm == "BEQ") {
            const char* cond = "0";
            if (nm == "BPL") cond = "!gN";
            else if (nm == "BMI") cond = "gN";
            else if (nm == "BVC") cond = "!gV";
            else if (nm == "BVS") cond = "gV";
            else if (nm == "BCC") cond = "!gC";
            else if (nm == "BCS") cond = "gC";
            else if (nm == "BNE") cond = "!gZ";
            else if (nm == "BEQ") cond = "gZ";
            int target = pc + 2 + (int8_t)b1;
            string tl = (target >= 0x1000 && target <= 0x1FFF)
                            ? label(bank, target) : "L_trap";
            out << "  tick(2);\n";
            out << "  if (" << cond << ") { tick(1); goto " << tl << "; }\n";
            out << "  goto " << nxt << ";\n}\n";
            return;
        }

        if (nm == "JMP") {
            if (op.mode == ABS) {
                int raw = b1 | (b2 << 8);
                out << "  tick(3);\n";
                if (raw & 0x1000)
                    out << "  goto " << label(bank, 0x1000 | (raw & 0xFFF)) << ";\n";
                else
                    out << "  goto L_trap;\n";
            } else {
                out << "  tick(5);\n";
                out << "  { u16 a = 0x" << hex4(b1 | (b2 << 8)) << ";\n";
                out << "    u16 t = (u16)(rd(a) | (rd((u16)((a & 0xFF00) | ((a + 1) & 0xFF))) << 8));\n";
                out << "    if (t & 0x1000) goto *(MAP[gBank][t & 0xFFF]);\n";
                out << "    goto L_trap; }\n";
            }
            out << "}\n";
            return;
        }

        if (nm == "JSR") {
            int raw = b1 | (b2 << 8);
            out << "  tick(6);\n";
            out << "  push16(0x" << hex4(pc + 2) << ");\n";
            if (raw & 0x1000)
                out << "  goto " << label(bank, 0x1000 | (raw & 0xFFF)) << ";\n";
            else
                out << "  goto L_trap;\n";
            out << "}\n";
            return;
        }

        if (nm == "RTS") {
            out << "  tick(6);\n";
            out << "  { u16 t = (u16)(pull16() + 1);\n";
            out << "    if (t & 0x1000) goto *(MAP[gBank][t & 0xFFF]);\n";
            out << "    goto L_trap; }\n";
            out << "}\n";
            return;
        }

        if (nm == "RTI") {
            out << "  tick(6);\n";
            out << "  { u8 f = pull8();\n";
            out << "    gC = f & 1; gZ = (f >> 1) & 1; gI = (f >> 2) & 1;\n";
            out << "    gD = (f >> 3) & 1; gV = (f >> 6) & 1; gN = (f >> 7) & 1;\n";
            out << "    u16 t = pull16();\n";
            out << "    if (t & 0x1000) goto *(MAP[gBank][t & 0xFFF]);\n";
            out << "    goto L_trap; }\n";
            out << "}\n";
            return;
        }

        if (nm == "BRK") {
            out << "  tick(7);\n";
            out << "  push16(0x" << hex4(pc + 2) << ");\n";
            out << "  push8((u8)(0x30 | gC | (gZ << 1) | (gI << 2) | (gD << 3) | (gV << 6) | (gN << 7)));\n";
            out << "  gI = 1;\n";
            out << "  { u16 t = (u16)(rd(0xFFFE) | (rd(0xFFFF) << 8));\n";
            out << "    if (t & 0x1000) goto *(MAP[gBank][t & 0xFFF]);\n";
            out << "    goto L_trap; }\n";
            out << "}\n";
            return;
        }

        // Undocumented read-modify-write opcodes
        if (nm == "DCP" || nm == "ISC" || nm == "SLO" || nm == "RLA" ||
            nm == "SRE" || nm == "RRA") {
            out << "  tick(" << cyc << ");\n";
            pre();
            out << "  u8 v = " << readExpr(addr) << ";\n";
            if (nm == "DCP") {
                out << "  v--;\n";
                out << "  " << writeStmt(addr, "v") << "\n";
                out << "  unsigned s = (unsigned)gA - v;\n";
                out << "  gC = (gA >= v); gZ = (gA == v); gN = ((u8)s >> 7);\n";
            } else if (nm == "ISC") {
                out << "  v++;\n";
                out << "  " << writeStmt(addr, "v") << "\n";
                out << "  sbc(v);\n";
            } else if (nm == "SLO") {
                out << "  gC = (v >> 7); v = (u8)(v << 1);\n";
                out << "  " << writeStmt(addr, "v") << "\n";
                out << "  gA = (u8)(gA | v); gZ = (gA == 0); gN = (gA >> 7);\n";
            } else if (nm == "RLA") {
                out << "  { u8 c = gC; gC = (v >> 7); v = (u8)((v << 1) | c); }\n";
                out << "  " << writeStmt(addr, "v") << "\n";
                out << "  gA = (u8)(gA & v); gZ = (gA == 0); gN = (gA >> 7);\n";
            } else if (nm == "SRE") {
                out << "  gC = (v & 1); v >>= 1;\n";
                out << "  " << writeStmt(addr, "v") << "\n";
                out << "  gA = (u8)(gA ^ v); gZ = (gA == 0); gN = (gA >> 7);\n";
            } else {
                out << "  { u8 c = gC; gC = (v & 1); v = (u8)((v >> 1) | (c << 7)); }\n";
                out << "  " << writeStmt(addr, "v") << "\n";
                out << "  adc(v);\n";
            }
            out << "  goto " << nxt << ";\n}\n";
            return;
        }

        // Implied / accumulator instructions
        out << "  tick(" << cyc << ");\n";
        if (nm == "NOP") {
            // nothing
        } else if (nm == "CLC") out << "  gC = 0;\n";
        else if (nm == "SEC") out << "  gC = 1;\n";
        else if (nm == "CLI") out << "  gI = 0;\n";
        else if (nm == "SEI") out << "  gI = 1;\n";
        else if (nm == "CLD") out << "  gD = 0;\n";
        else if (nm == "SED") out << "  gD = 1;\n";
        else if (nm == "CLV") out << "  gV = 0;\n";
        else if (nm == "TAX") out << "  gX = gA; gZ = (gX == 0); gN = (gX >> 7);\n";
        else if (nm == "TAY") out << "  gY = gA; gZ = (gY == 0); gN = (gY >> 7);\n";
        else if (nm == "TXA") out << "  gA = gX; gZ = (gA == 0); gN = (gA >> 7);\n";
        else if (nm == "TYA") out << "  gA = gY; gZ = (gA == 0); gN = (gA >> 7);\n";
        else if (nm == "TSX") out << "  gX = gS; gZ = (gX == 0); gN = (gX >> 7);\n";
        else if (nm == "TXS") out << "  gS = gX;\n";
        else if (nm == "INX") out << "  gX++; gZ = (gX == 0); gN = (gX >> 7);\n";
        else if (nm == "INY") out << "  gY++; gZ = (gY == 0); gN = (gY >> 7);\n";
        else if (nm == "DEX") out << "  gX--; gZ = (gX == 0); gN = (gX >> 7);\n";
        else if (nm == "DEY") out << "  gY--; gZ = (gY == 0); gN = (gY >> 7);\n";
        else if (nm == "PHA") out << "  push8(gA);\n";
        else if (nm == "PLA") out << "  gA = pull8(); gZ = (gA == 0); gN = (gA >> 7);\n";
        else if (nm == "PHP")
            out << "  push8((u8)(0x30 | gC | (gZ << 1) | (gI << 2) | (gD << 3) | (gV << 6) | (gN << 7)));\n";
        else if (nm == "PLP") {
            out << "  { u8 f = pull8();\n";
            out << "    gC = f & 1; gZ = (f >> 1) & 1; gI = (f >> 2) & 1;\n";
            out << "    gD = (f >> 3) & 1; gV = (f >> 6) & 1; gN = (f >> 7) & 1; }\n";
        }
        out << "  goto " << nxt << ";\n}\n";
    }

    bool run(const string& romPath, const string& outPath) {
        FILE* fp = std::fopen(romPath.c_str(), "rb");
        if (!fp) {
            std::fprintf(stderr, "cannot open %s\n", romPath.c_str());
            return false;
        }
        std::fseek(fp, 0, SEEK_END);
        long sz = std::ftell(fp);
        std::fseek(fp, 0, SEEK_SET);
        if (sz != 4096 && sz != 8192) {
            std::fprintf(stderr, "unsupported ROM size %ld (need 4096 or 8192)\n", sz);
            std::fclose(fp);
            return false;
        }
        rom.resize((size_t)sz);
        if (std::fread(rom.data(), 1, rom.size(), fp) != rom.size()) {
            std::fprintf(stderr, "short read\n");
            std::fclose(fp);
            return false;
        }
        std::fclose(fp);
        romSize = (int)rom.size();
        banks = romSize / 4096;

        int startBank = 0;
        if (banks == 2) {
            startBank = 1; // F8 hardware powers up in bank 1
            int v = rom[4096 + 0xFFC] | (rom[4096 + 0xFFD] << 8);
            if (!(v & 0x1000)) startBank = 0;
        }
        int resetRaw = rom[startBank * 4096 + 0xFFC] |
                       (rom[startBank * 4096 + 0xFFD] << 8);
        if (!(resetRaw & 0x1000)) {
            std::fprintf(stderr, "warning: reset vector %04X does not point into ROM, using F000\n",
                         resetRaw);
            resetRaw = 0xF000;
        }
        int reset = 0x1000 | (resetRaw & 0xFFF);

        out.open(outPath, std::ios::binary);
        if (!out) {
            std::fprintf(stderr, "cannot write %s\n", outPath.c_str());
            return false;
        }

        out << "// Generated by VCSRelink. Do not edit.\n";
        out << "#include \"runtime.hpp\"\n\n";
        out << "namespace rt {\n";
        out << "u8 ROM_DATA[" << (banks * 4096) << "] = {\n";
        for (int i = 0; i < banks * 4096; i++) {
            if ((i & 15) == 0) out << "  ";
            char b[8];
            std::snprintf(b, sizeof b, "0x%02X", rom[(size_t)i]);
            out << b;
            if (i + 1 < banks * 4096) out << ",";
            if ((i & 15) == 15) out << "\n";
        }
        out << "};\n";
        out << "int ROM_BANKS = " << banks << ";\n";
        out << "void* MAP[2][0x1000];\n";
        out << "} // namespace rt\n\n";
        out << "using namespace rt;\n\n";
        out << "extern \"C\" void relink_cpu_exec() {\n";
        out << "  static bool inited = false;\n";
        out << "  if (!inited) {\n";
        out << "    inited = true;\n";
        for (int b = 0; b < banks; b++) {
            for (int off = 0; off < 0x1000; off++) {
                out << "    MAP[" << b << "][0x" << hex4(off) << "] = &&"
                    << label(b, 0x1000 + off) << ";\n";
            }
        }
        out << "  }\n";
        out << "  gBank = " << startBank << ";\n";
        out << "  goto " << label(startBank, reset) << ";\n";

        for (int b = 0; b < banks; b++) {
            for (int off = 0; off < 0x1000; off++) {
                emitInstruction(b, 0x1000 + off);
            }
        }

        out << "L_trap: {\n";
        out << "  u16 t = (u16)(rd(0xFFFE) | (rd(0xFFFF) << 8));\n";
        out << "  if (t & 0x1000) goto *(MAP[gBank][t & 0xFFF]);\n";
        out << "  fatal(\"unrecoverable jump\", 0);\n";
        out << "}\n";
        out << "}\n";
        out.close();
        std::fprintf(stderr, "relinked %s: %d bank(s), reset=%04X, start bank=%d\n",
                     romPath.c_str(), banks, reset, startBank);
        return true;
    }
};

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: %s <rom.bin> <out.cpp>\n", argv[0]);
        return 1;
    }
    buildTable();
    Emitter e;
    if (!e.run(argv[1], argv[2])) return 1;
    return 0;
}
