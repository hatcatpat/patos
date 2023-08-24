#include "x.h"

#define NUM_KEYS 82

uint8_t scancode2char[NUM_KEYS]
    = { 0,   0x1b, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-',  '=',
        8,   '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[',  ']',
        0xd, 0,    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,    '*',
        0,   0x20, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    '7',
        '8', '9',  '-', '4', '5', '6', '+', '1', '2', '3', '0', '.' };

uint8_t
char2scancode (char c)
{
    uint8_t i;
    for (i = 0; i < NUM_KEYS; ++i)
        if (scancode2char[i] == c)
            return i;
    return 0;
}
