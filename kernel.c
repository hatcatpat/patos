#include "font.h"

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long int int64_t;
typedef unsigned long int uint64_t;
typedef unsigned int uint_t;

enum booleans
{
    false,
    true
};

typedef unsigned char bool;

#define W 320
#define H 200
#define FONTW (W / 4)
#define FONTH (H / 4)

char *vga = (char *)0xa0000;
uint8_t buffer[W * H];
int cursor[2] = { 0, 0 };     /* x, y */
uint8_t color[2] = { 15, 0 }; /* fg, bg */
bool keys[0xff];
uint_t ticks = 0;

void
halt (void)
{
    __asm__ ("hlt");
}

void
forever (void)
{
    for (;;)
        halt ();
}

void
memcpy (void *dest, void *src, uint_t count)
{
    __asm__ ("rep movsb" : "+c"(count), "+S"(src), "+D"(dest));
}

void
memset (void *dest, int value, uint_t count)
{
    __asm__ ("rep stosb" : "+c"(count), "+a"(value), "+D"(dest));
}

void
paint (void)
{
    memcpy (vga, buffer, W * H);
}

void
clear (char bg)
{
    memset (buffer, bg, W * H);
    cursor[0] = cursor[1] = 0;
    color[1] = bg;
}

void
putpixel (int16_t x, int16_t y, uint8_t col)
{
    if (y < H && x < W)
        buffer[y * W + x] = col;
}

void
putrect (int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t col)
{
    static uint16_t i, j;
    for (i = 0; i < w; ++i)
        for (j = 0; j < h; ++j)
            putpixel (x + i, y + j, col);
}

void
cursornext (void)
{
    cursor[0]++;
    if (cursor[0] > FONTW)
        cursor[0] = 0, cursor[1]++;
}

void
cursornewline (void)
{
    cursor[0] = 0;
    cursor[1]++;
}

void
cursortab (void)
{
    cursornext (), cursornext (), cursornext (), cursornext ();
}

void
putc (char c)
{
    if (c == '\n')
        cursornewline ();
    else if (c == '\t')
        cursortab ();
    else
        {
            char *font = 0;
            if ('a' <= c && c <= 'z')
                font = (char *)alpha[c - 'a'];
            else if ('0' <= c && c <= '9')
                font = (char *)digit[c - '0'];

            if (font == 0)
                putrect (cursor[0] * 4, cursor[1] * 4, 4, 4, color[1]);
            else
                {
                    int i, j;
                    char x;
                    for (i = 0; i < 4; i++)
                        for (j = 0; j < 4; j++)
                            if ((x = font[i + j * 4]) != 0)
                                putpixel (cursor[0] * 4 + i, cursor[1] * 4 + j,
                                          color[0] * x);
                }

            cursornext ();
        }
}

void
puts (char *str)
{
    while (*str != 0)
        putc (*str++);
}

uint_t
strlen (char *str)
{
    uint_t i = 0;
    while (str[i++] != 0)
        ;
    return i;
}

void
putn (uint_t n)
{
    static char arr[10];
    uint_t i = 0;
    do
        {
            arr[i++] = n % 10 + '0';
            n /= 10;
        }
    while (n != 0);
    while (i != 0)
        putc (arr[--i]);
}

void
putnh (uint_t n)
{
    static char arr[8];
    uint_t i = 0;
    do
        {
            arr[i++] = n % 16;
            n /= 16;
        }
    while (n != 0);
    while (i != 0)
        {
            char c = arr[--i];
            if (0 <= c && c <= 9)
                putc (c + '0');
            else
                putc (c - 10 + 'a');
        }
}

uint8_t
inb (uint16_t port)
{
    static uint8_t result;
    __asm__ ("in al, dx" : "=a"(result) : "d"(port));
    return result;
}

void
outb (uint16_t port, uint8_t data)
{
    __asm__ ("out dx, al" : : "d"(port), "a"(data));
}

void
com1 (uint8_t data)
{
    outb (0x3f8, data);
}

void
io_wait (void)
{
    outb (0x80, 0);
}

void
sleep (uint_t dur)
{
    static uint_t end;
    end = ticks + dur;
    while (ticks < end)
        halt (); /* TODO: test this */
}

enum exceptions
{
    E_DIVIDE_BY_ZERO,
    E_NUM_EXCEPTIONS
};

void
exception (uint8_t e)
{
    clear (4);
    puts ("an exception has occured\t");
    putn (e);
    paint ();
    forever ();
}

enum irqs
{
    I_TIMER,
    I_KEYBOARD,
    I_NUM_IRQS
};

void
irq (uint8_t e)
{
    switch (e)
        {
        case I_TIMER:
            ticks++;
            break;

        case I_KEYBOARD:
            {
                uint8_t scancode = inb (0x60);
                keys[0] = scancode & 0x80 ? false : true;
            }
            break;
        }

    if (e >= 8)
        outb (0xa0, 0x20);
    outb (0x20, 0x20);
}

void
mask_irq (uint8_t x, bool enable)
{
    uint16_t port;
    uint8_t value;

    if (x < 8)
        port = 0x21;
    else
        port = 0xa1, x -= 8;

    if (enable)
        value = inb (port) & ~(1 << x);
    else
        value = inb (port) | (1 << x);

    outb (port, value);
}

void
remap_irq (void)
{
    outb (0x20, 0x11);
    outb (0xa0, 0x11);

    outb (0x21, 32);
    outb (0xa1, 40);

    outb (0x21, 4);
    outb (0xa1, 2);

    outb (0x21, 1);
    outb (0xa1, 1);

    outb (0x21, 0xff);
    outb (0xa1, 0xff);

    mask_irq (I_TIMER, true);
    mask_irq (I_KEYBOARD, true);
}

void
init_timer (uint32_t hz)
{
    uint32_t divisor = hz == 0 ? 0 : 1193182 / hz;
    outb (0x43, 54); /* 00_11_011_0 = 54 */
    outb (0x40, divisor & 0xff);
    outb (0x40, (divisor >> 8) & 0xff);
    ticks = 0;
}

void
kernel (void)
{
    clear (2);
    paint ();
    remap_irq ();
    init_timer (100);
    __asm__ ("sti");

    for (;;)
        {
            static int x = 4, y = 17, dx = 0, dy = 0;
            char *hello = "hello";
            clear (keys[0] ? 8 : 4);
            cursor[0] = x, cursor[1] = y;
            puts (hello);
            cursor[0] = cursor[1] = 0;
            putn (ticks);
            paint ();

            if (dx == 0 && x == 0)
                dx = 1;
            else if (dx == 1 && x == FONTW - strlen (hello) + 1)
                dx = 0;

            if (dx == 0)
                x--;
            else if (dx == 1)
                x++;

            if (dy == 0 && y == 0)
                dy = 1;
            else if (dy == 1 && y == FONTH - 1)
                dy = 0;

            if (dy == 0)
                y--;
            else if (dy == 1)
                y++;

            sleep (1);
        }

    forever ();
}
