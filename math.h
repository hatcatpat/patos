#include "x.h"

#define CLAMP(x, a, b) ((x) < (a) ? (a) : (x) > (b) ? (b) : (x))

static uint_t next = 1;

#define RAND_MAX 32767

uint_t
random (void)
{
    next = next * 1103515245 + 12345;
    return (next / 65536) % 32768;
}

void
seed (uint_t seed)
{
    next = seed;
}
