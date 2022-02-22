#include "net.h"

#ifdef MODULE_CURR
#undef MODULE_CURR
#define MODULE_CURR MODULE_NET_BASE64
#else
#define MODULE_CURR MODULE_NET_BASE64
#endif
#define CHAR62 '+'
#define CHAR63 '/'
#define CHARPAD '='

static const char e0[256] = {
 'A',  'A',  'A',  'A',  'B',  'B',  'B',  'B',  'C',  'C',
 'C',  'C',  'D',  'D',  'D',  'D',  'E',  'E',  'E',  'E',
 'F',  'F',  'F',  'F',  'G',  'G',  'G',  'G',  'H',  'H',
 'H',  'H',  'I',  'I',  'I',  'I',  'J',  'J',  'J',  'J',
 'K',  'K',  'K',  'K',  'L',  'L',  'L',  'L',  'M',  'M',
 'M',  'M',  'N',  'N',  'N',  'N',  'O',  'O',  'O',  'O',
 'P',  'P',  'P',  'P',  'Q',  'Q',  'Q',  'Q',  'R',  'R',
 'R',  'R',  'S',  'S',  'S',  'S',  'T',  'T',  'T',  'T',
 'U',  'U',  'U',  'U',  'V',  'V',  'V',  'V',  'W',  'W',
 'W',  'W',  'X',  'X',  'X',  'X',  'Y',  'Y',  'Y',  'Y',
 'Z',  'Z',  'Z',  'Z',  'a',  'a',  'a',  'a',  'b',  'b',
 'b',  'b',  'c',  'c',  'c',  'c',  'd',  'd',  'd',  'd',
 'e',  'e',  'e',  'e',  'f',  'f',  'f',  'f',  'g',  'g',
 'g',  'g',  'h',  'h',  'h',  'h',  'i',  'i',  'i',  'i',
 'j',  'j',  'j',  'j',  'k',  'k',  'k',  'k',  'l',  'l',
 'l',  'l',  'm',  'm',  'm',  'm',  'n',  'n',  'n',  'n',
 'o',  'o',  'o',  'o',  'p',  'p',  'p',  'p',  'q',  'q',
 'q',  'q',  'r',  'r',  'r',  'r',  's',  's',  's',  's',
 't',  't',  't',  't',  'u',  'u',  'u',  'u',  'v',  'v',
 'v',  'v',  'w',  'w',  'w',  'w',  'x',  'x',  'x',  'x',
 'y',  'y',  'y',  'y',  'z',  'z',  'z',  'z',  '0',  '0',
 '0',  '0',  '1',  '1',  '1',  '1',  '2',  '2',  '2',  '2',
 '3',  '3',  '3',  '3',  '4',  '4',  '4',  '4',  '5',  '5',
 '5',  '5',  '6',  '6',  '6',  '6',  '7',  '7',  '7',  '7',
 '8',  '8',  '8',  '8',  '9',  '9',  '9',  '9',  '+',  '+',
 '+',  '+',  '/',  '/',  '/',  '/'
};
static const char e1[256] = {
 'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',
 'K',  'L',  'M',  'N',  'O',  'P',  'Q',  'R',  'S',  'T',
 'U',  'V',  'W',  'X',  'Y',  'Z',  'a',  'b',  'c',  'd',
 'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',
 'o',  'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',
 'y',  'z',  '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
 '8',  '9',  '+',  '/',  'A',  'B',  'C',  'D',  'E',  'F',
 'G',  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',  'P',
 'Q',  'R',  'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z',
 'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',
 'k',  'l',  'm',  'n',  'o',  'p',  'q',  'r',  's',  't',
 'u',  'v',  'w',  'x',  'y',  'z',  '0',  '1',  '2',  '3',
 '4',  '5',  '6',  '7',  '8',  '9',  '+',  '/',  'A',  'B',
 'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L',
 'M',  'N',  'O',  'P',  'Q',  'R',  'S',  'T',  'U',  'V',
 'W',  'X',  'Y',  'Z',  'a',  'b',  'c',  'd',  'e',  'f',
 'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',  'p',
 'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',
 '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',
 '+',  '/',  'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',
 'I',  'J',  'K',  'L',  'M',  'N',  'O',  'P',  'Q',  'R',
 'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z',  'a',  'b',
 'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',
 'm',  'n',  'o',  'p',  'q',  'r',  's',  't',  'u',  'v',
 'w',  'x',  'y',  'z',  '0',  '1',  '2',  '3',  '4',  '5',
 '6',  '7',  '8',  '9',  '+',  '/'
};
static const char e2[256] = {
 'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',
 'K',  'L',  'M',  'N',  'O',  'P',  'Q',  'R',  'S',  'T',
 'U',  'V',  'W',  'X',  'Y',  'Z',  'a',  'b',  'c',  'd',
 'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',
 'o',  'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',
 'y',  'z',  '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
 '8',  '9',  '+',  '/',  'A',  'B',  'C',  'D',  'E',  'F',
 'G',  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',  'P',
 'Q',  'R',  'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z',
 'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',
 'k',  'l',  'm',  'n',  'o',  'p',  'q',  'r',  's',  't',
 'u',  'v',  'w',  'x',  'y',  'z',  '0',  '1',  '2',  '3',
 '4',  '5',  '6',  '7',  '8',  '9',  '+',  '/',  'A',  'B',
 'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L',
 'M',  'N',  'O',  'P',  'Q',  'R',  'S',  'T',  'U',  'V',
 'W',  'X',  'Y',  'Z',  'a',  'b',  'c',  'd',  'e',  'f',
 'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',  'p',
 'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',
 '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',
 '+',  '/',  'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',
 'I',  'J',  'K',  'L',  'M',  'N',  'O',  'P',  'Q',  'R',
 'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z',  'a',  'b',
 'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',
 'm',  'n',  'o',  'p',  'q',  'r',  's',  't',  'u',  'v',
 'w',  'x',  'y',  'z',  '0',  '1',  '2',  '3',  '4',  '5',
 '6',  '7',  '8',  '9',  '+',  '/'
};

/* SPECIAL DECODE TABLES FOR LITTLE ENDIAN (INTEL) CPUS */
static const uint32_t d0[256] = {
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x000000f8, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x000000fc,
0x000000d0, 0x000000d4, 0x000000d8, 0x000000dc, 0x000000e0, 0x000000e4,
0x000000e8, 0x000000ec, 0x000000f0, 0x000000f4, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x00000000,
0x00000004, 0x00000008, 0x0000000c, 0x00000010, 0x00000014, 0x00000018,
0x0000001c, 0x00000020, 0x00000024, 0x00000028, 0x0000002c, 0x00000030,
0x00000034, 0x00000038, 0x0000003c, 0x00000040, 0x00000044, 0x00000048,
0x0000004c, 0x00000050, 0x00000054, 0x00000058, 0x0000005c, 0x00000060,
0x00000064, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x00000068, 0x0000006c, 0x00000070, 0x00000074, 0x00000078,
0x0000007c, 0x00000080, 0x00000084, 0x00000088, 0x0000008c, 0x00000090,
0x00000094, 0x00000098, 0x0000009c, 0x000000a0, 0x000000a4, 0x000000a8,
0x000000ac, 0x000000b0, 0x000000b4, 0x000000b8, 0x000000bc, 0x000000c0,
0x000000c4, 0x000000c8, 0x000000cc, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff
};


static const uint32_t d1[256] = {
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x0000e003, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x0000f003,
0x00004003, 0x00005003, 0x00006003, 0x00007003, 0x00008003, 0x00009003,
0x0000a003, 0x0000b003, 0x0000c003, 0x0000d003, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x00000000,
0x00001000, 0x00002000, 0x00003000, 0x00004000, 0x00005000, 0x00006000,
0x00007000, 0x00008000, 0x00009000, 0x0000a000, 0x0000b000, 0x0000c000,
0x0000d000, 0x0000e000, 0x0000f000, 0x00000001, 0x00001001, 0x00002001,
0x00003001, 0x00004001, 0x00005001, 0x00006001, 0x00007001, 0x00008001,
0x00009001, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x0000a001, 0x0000b001, 0x0000c001, 0x0000d001, 0x0000e001,
0x0000f001, 0x00000002, 0x00001002, 0x00002002, 0x00003002, 0x00004002,
0x00005002, 0x00006002, 0x00007002, 0x00008002, 0x00009002, 0x0000a002,
0x0000b002, 0x0000c002, 0x0000d002, 0x0000e002, 0x0000f002, 0x00000003,
0x00001003, 0x00002003, 0x00003003, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff
};


static const uint32_t d2[256] = {
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x00800f00, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x00c00f00,
0x00000d00, 0x00400d00, 0x00800d00, 0x00c00d00, 0x00000e00, 0x00400e00,
0x00800e00, 0x00c00e00, 0x00000f00, 0x00400f00, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x00000000,
0x00400000, 0x00800000, 0x00c00000, 0x00000100, 0x00400100, 0x00800100,
0x00c00100, 0x00000200, 0x00400200, 0x00800200, 0x00c00200, 0x00000300,
0x00400300, 0x00800300, 0x00c00300, 0x00000400, 0x00400400, 0x00800400,
0x00c00400, 0x00000500, 0x00400500, 0x00800500, 0x00c00500, 0x00000600,
0x00400600, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x00800600, 0x00c00600, 0x00000700, 0x00400700, 0x00800700,
0x00c00700, 0x00000800, 0x00400800, 0x00800800, 0x00c00800, 0x00000900,
0x00400900, 0x00800900, 0x00c00900, 0x00000a00, 0x00400a00, 0x00800a00,
0x00c00a00, 0x00000b00, 0x00400b00, 0x00800b00, 0x00c00b00, 0x00000c00,
0x00400c00, 0x00800c00, 0x00c00c00, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff
};


static const uint32_t d3[256] = {
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x003e0000, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x003f0000,
0x00340000, 0x00350000, 0x00360000, 0x00370000, 0x00380000, 0x00390000,
0x003a0000, 0x003b0000, 0x003c0000, 0x003d0000, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x00000000,
0x00010000, 0x00020000, 0x00030000, 0x00040000, 0x00050000, 0x00060000,
0x00070000, 0x00080000, 0x00090000, 0x000a0000, 0x000b0000, 0x000c0000,
0x000d0000, 0x000e0000, 0x000f0000, 0x00100000, 0x00110000, 0x00120000,
0x00130000, 0x00140000, 0x00150000, 0x00160000, 0x00170000, 0x00180000,
0x00190000, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x001a0000, 0x001b0000, 0x001c0000, 0x001d0000, 0x001e0000,
0x001f0000, 0x00200000, 0x00210000, 0x00220000, 0x00230000, 0x00240000,
0x00250000, 0x00260000, 0x00270000, 0x00280000, 0x00290000, 0x002a0000,
0x002b0000, 0x002c0000, 0x002d0000, 0x002e0000, 0x002f0000, 0x00300000,
0x00310000, 0x00320000, 0x00330000, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff,
0x01ffffff, 0x01ffffff, 0x01ffffff, 0x01ffffff
};

#define BADCHAR 0x01FFFFFF

#define BASE64_ENCODE_BUF_SIZE(size) ( 4 * ( (size) / 3 )  + ( (size) % 3 == 0 ? 0 : 4 ) )
int base64_encode(const char *str,unsigned int strSize,char **dist,unsigned int *distSize){
    int ret = 0;
    R(NULL == str);
    R(0 >= strSize);
    R(NULL == dist);
    R(NULL != (*dist));
    R(NULL == distSize);

    unsigned int bufSize = BASE64_ENCODE_BUF_SIZE(strSize);
    char *buf = NULL;
    R(MALLOC_TN(&buf,str_t,bufSize));
    char *ptr = buf;

    uint8_t t1, t2, t3;
    int i = 0;
    if( strSize > 2 ){
        for(; i < strSize - 2; i += 3){
            t1 = str[i]; t2 = str[i+1]; t3 = str[i+2];
            *ptr++ = e0[t1];
            *ptr++ = e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
            *ptr++ = e1[((t2 & 0x0F) << 2) | ((t3 >> 6) & 0x03)];
            *ptr++ = e2[t3];
        }
    }

    if( 0 == strSize - i ){
        
    }else if( 1 == strSize - i ){
        t1 = str[i];
        *ptr++ = e0[t1];
        *ptr++ = e1[(t1 & 0x03) << 4];
        *ptr++ = CHARPAD;
        *ptr++ = CHARPAD;
    }else{
        t1 = str[i]; t2 = str[i+1];
        *ptr++ = e0[t1];
        *ptr++ = e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
        *ptr++ = e2[(t2 & 0x0F) << 2];
        *ptr++ = CHARPAD;
    }

    (*dist) = buf;
    (*distSize) = bufSize;
    return ret;
}

int base64_decode(const char *str,unsigned int strSize,char **dist,unsigned int *distSize){
    int ret = 0,errRet = 0;
    R(NULL == str);
    R(0 >= strSize);
    R(NULL == dist);
    R(NULL != (*dist));
    R(NULL == distSize);

realStrSize:
    if( str[strSize - 1] == '\0' ){
        strSize--;
        goto realStrSize;
    }

    unsigned int bufTmpSize = ( 3 * ( strSize / 4 )  + ( strSize % 4 == 0 ? 0 : 4) );
    char *bufTmp = NULL;
    R(MALLOC_TN(&bufTmp,str_t,bufTmpSize));
    char *p = bufTmp;

    unsigned int dataSize = 0;
    char *data = NULL;

    size_t i;
    int leftover = strSize % 4;
    size_t chunks = (leftover == 0) ? (strSize / 4 - 1) : strSize / 4;

    uint32_t x = 0;
    const uint8_t* y = (uint8_t*)str;
    for (i = 0; i < chunks; ++i, y += 4) {
        x = d0[y[0]] | d1[y[1]] | d2[y[2]] | d3[y[3]];
        /* G_E(x >= BADCHAR); */
        *p++ =  ((uint8_t*)(&x))[0];
        *p++ =  ((uint8_t*)(&x))[1];
        *p++ =  ((uint8_t*)(&x))[2];
    }

    switch (leftover) {
    case 0:
        x = d0[y[0]] | d1[y[1]] | d2[y[2]] | d3[y[3]];
        /* G_E(x >= BADCHAR); */
        *p++ =  ((uint8_t*)(&x))[0];
        *p++ =  ((uint8_t*)(&x))[1];
        *p =    ((uint8_t*)(&x))[2];
        dataSize = (chunks+1)*3;
        break;
    case 1:  /* with padding this is an impossible case */
        x = d0[y[0]];
        *p = *((uint8_t*)(&x)); /* i.e. first char/byte in int */
        break;
    case 2: /* case 2, 1  output byte */
        x = d0[y[0]] | d1[y[1]];
        *p = *((uint8_t*)(&x)); /* i.e. first char */
        break;
    default: /* case 3, 2 output bytes */
        x = d0[y[0]] | d1[y[1]] | d2[y[2]];  /* 0x3c */
        *p++ =  ((uint8_t*)(&x))[0];
        *p =  ((uint8_t*)(&x))[1];
        break;
    }
    if( leftover != 0 ){
        /* G_E(x >= BADCHAR);*/
        dataSize = 3*chunks + (6*leftover)/8;
    }

    R(MALLOC_TN(dist,str_t,dataSize));
    memcpy((*dist),bufTmp,dataSize);
    R(FREE_TN(&bufTmp,str_t,bufTmpSize));
    (*distSize) = dataSize;

    return ret;

error:
    errRet = ret;
    if(NULL != bufTmp) R(FREE_TN(&bufTmp,str_t,bufTmpSize));
    return errRet;
}
