#include "kernel.h"

#define W 320
#define H 200
#define FONTW (W / 4)
#define FONTH (H / 4)

#define TIMER_HZ 1193180

char *vga = (char *)0xa0000;
uint8_t buffer[W * H];
int cursor[2] = { 0, 0 };     /* x, y */
uint8_t color[2] = { 15, 0 }; /* fg, bg */
uint_t ticks = 0, beep_dur = 0;
bool keys[0xff];

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
copy (void *dest, void *src, uint_t count)
{
    __asm__ ("rep movsb" : "+c"(count), "+S"(src), "+D"(dest));
}

void
set (void *dest, int value, uint_t count)
{
    __asm__ ("rep stosb" : "+c"(count), "+a"(value), "+D"(dest));
}

void
paint (void)
{
    copy (vga, buffer, W * H);
}

void
clear (char bg)
{
    set (buffer, bg, W * H);
    cursor[0] = cursor[1] = 0;
    color[1] = bg;
}

void
putpixel (int16_t x, int16_t y, uint8_t col)
{
    if (0 <= y && y < H && 0 <= x && x < W)
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
            else if ('A' <= c && c <= 'Z')
                font = (char *)alpha[c - 'A'];
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
        halt ();
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
    I_KEYBOARD
};

void
irq (uint8_t e)
{
    switch (e)
        {
        case I_TIMER:
            ticks++;
            if (beep_dur != 0)
                {
                    beep_dur--;
                    if (beep_dur == 0)
                        nobeep ();
                }
            break;

        case I_KEYBOARD:
            {
                uint8_t scancode = inb (0x60);
                uint8_t key = scancode & ~0x80;
                keys[key] = !(scancode & 0x80);
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
}

void
timer (uint8_t chan, uint32_t hz)
{
    uint32_t div = hz == 0 ? 0 : TIMER_HZ / hz;

    switch (chan)
        {
        case 0:
            outb (0x43, 0x36);
            outb (0x40, div);
            outb (0x40, div >> 8);
            break;

        case 2:
            outb (0x43, 0xb6);
            outb (0x42, div);
            outb (0x42, div >> 8);
            break;
        }
}

void
beep (uint_t freq, uint_t dur)
{
    timer (2, freq);
    beep_dur = 10;
}

void
nobeep ()
{
    timer (2, TIMER_HZ);
}

void
sound ()
{
    nobeep ();
    outb (0x61, inb (0x61) | 3);
}

void
nosound (void)
{
    outb (0x61, inb (0x61) & 0xfc);
}

void
bounce (void)
{
    static int x = FONTW / 2, y = FONTH / 2, dx = 0, dy = 0;
    static char *hello = "hello";
    for (;;)
        {
            clear (keys[1] ? 8 : 4);
            cursor[0] = x, cursor[1] = y;
            puts (hello);
            cursor[0] = cursor[1] = 0;
            putn (ticks);
            paint ();

            if (dx == 0 && x == 0)
                {
                    dx = 1;
                    beep (440, 20);
                }
            else if (dx == 1 && x == FONTW - strlen (hello) + 1)
                {
                    dx = 0;
                    beep (220, 20);
                }

            if (dx == 0)
                x--;
            else if (dx == 1)
                x++;

            if (dy == 0 && y == 0)
                {
                    dy = 1;
                    beep (660, 20);
                }
            else if (dy == 1 && y == FONTH - 1)
                {
                    dy = 0;
                    beep (770, 20);
                }

            if (dy == 0)
                y--;
            else if (dy == 1)
                y++;

            sleep (1);
        }
}

void
kernel (void)
{
    clear (2);
    paint ();

    remap_irq ();
    mask_irq (I_TIMER, true);
    mask_irq (I_KEYBOARD, true);
    timer (0, 100);
    sound ();

    __asm__ ("sti");

    bounce ();

    forever ();
}
