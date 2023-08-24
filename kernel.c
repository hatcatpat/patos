#include "kernel.h"

#include "fun.c"

#define TIMER_HZ 1193180

char *vga = (char *)0xa0000;
uint8_t buffer[W * H];
int cursor[2] = { 0, 0 };        /* x, y */
uint8_t color[2] = { 0, 15 };    /* bg, fg */
bool keys[2][0xff];              /* [0][...] = recent, [1][...] = old */
bool buttons[2][3];              /* [0][...] = recent, [1][...] = old */
int mouse[2] = { W / 2, H / 2 }; /* x, y */
uint_t ticks = 0, beep_dur = 0;

void
halt (void)
{
    asm ("hlt");
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
    asm ("rep movsb" : "+c"(count), "+S"(src), "+D"(dest));
}

void
set (void *dest, int value, uint_t count)
{
    asm ("rep stosb" : "+c"(count), "+a"(value), "+D"(dest));
}

void
paint (void)
{
    copy (vga, buffer, W * H);
}

void
clear (uint8_t bg)
{
    set (buffer, bg, W * H);
    cursor[0] = cursor[1] = 0;
    color[0] = bg;
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
    uint_t i;
    for (i = 0; i < w * h; ++i)
        putpixel (x + i % w, y + i / w, col);
}

void
putellipse (int16_t cx, int16_t cy, uint16_t w, uint16_t h, uint8_t col)
{
    int hh = SQR (h);
    int ww = SQR (w);
    int hhww = hh * ww;
    int x0 = w;
    int dx = 0;
    int x, y, x1;

    for (x = -w; x <= w; x++)
        putpixel (cx + x, cy, col);

    for (y = 1; y <= h; y++)
        {
            x1 = x0 - (dx - 1);
            while (x1 > 0)
                {
                    if (SQR (x1) * hh + SQR (y) * ww <= hhww)
                        break;
                    x1--;
                }
            dx = x0 - x1;
            x0 = x1;

            for (x = -x0; x <= x0; x++)
                {
                    putpixel (cx + x, cy - y, col);
                    putpixel (cx + x, cy + y, col);
                }
        }
}

void
putbm (uint8_t *bm, int16_t x, int16_t y, uint8_t w, uint8_t h)
{
    uint_t i;
    for (i = 0; i < w * h; i++)
        if (bm[i] != 0)
            putpixel (x + i % w, y + i / w, bm[i]);
}

void
putbms (uint8_t *bm, uint_t sx, uint_t sy, int16_t x, int16_t y, uint8_t w,
        uint8_t h)
{
    uint_t i;
    for (i = 0; i < w * h; i++)
        if (bm[i] != 0)
            putrect (x + sx * (i % w), y + sy * (i / w), sx, sy, bm[i]);
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
                putrect (cursor[0] * 4 * font_scale,
                         cursor[1] * 4 * font_scale, 4, 4, color[0]);
            else
                {
                    uint8_t i;
                    for (i = 0; i < 4 * 4; ++i)
                        if (font[i] != 0)
                            putrect (cursor[0] * 4 * font_scale
                                         + font_scale * (i % 4),
                                     cursor[1] * 4 * font_scale
                                         + font_scale * (i / 4),
                                     font_scale, font_scale, color[1]);
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
slen (char *str)
{
    uint_t i = 0;
    while (str[i++] != 0)
        ;
    return i;
}

void
putn (uint_t n)
{
    char arr[10];
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
    char arr[8];
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
    uint8_t result;
    asm ("in al, dx" : "=a"(result) : "d"(port));
    return result;
}

void
outb (uint16_t port, uint8_t data)
{
    asm ("out dx, al" : : "d"(port), "a"(data));
}

void
outw (uint16_t port, uint16_t data)
{
    asm ("out dx, al" : : "d"(port), "a"(data));
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
    uint_t end = ticks + dur;
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
    I_KEYBOARD,
    I_MOUSE = 12
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
                keys[0][key] = !(scancode & 0x80);
            }
            break;

        case I_MOUSE:
            {
                static uint8_t packet[3] = { 0 };
                static uint8_t count = 0;

                if ((inb (0x64) & 1) != 0)
                    {
                        switch (count)
                            {
                            case 0:
                                packet[count++] = inb (0x60);
                                break;
                            case 1:
                                packet[count++] = inb (0x60);
                                break;
                            case 2:
                                packet[count++] = inb (0x60);
                                count = 0;

                                buttons[0][0] = packet[0] & 1;
                                buttons[0][1] = packet[0] & 2;
                                buttons[0][2] = packet[0] & 4;

                                /* maths magic from osdev wiki */
                                mouse[0]
                                    += packet[1] - (packet[0] << 4 & 0x100);
                                mouse[1]
                                    += (packet[0] << 3 & 0x100) - packet[2];
                                break;
                            }
                    }
            }
            break;
        }

    if (e >= 8)
        outb (0xa0, 0x20);
    outb (0x20, 0x20);
}

void
toggle_irq (uint8_t x, bool enable)
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

    outb (0x21, 0xff & ~(1 << 2));
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
    beep_dur = dur;
}

void
nobeep (void)
{
    timer (2, TIMER_HZ);
}

void
sound (void)
{
    nobeep ();
    outb (0x61, inb (0x61) | 3);
}

void
nosound (void)
{
    outb (0x61, inb (0x61) & 0xfc);
}

bool
pressed (uint8_t key)
{
    return !keys[1][key] && keys[0][key];
}

bool
held (uint8_t key)
{
    return keys[0][key];
}

bool
mheld (uint8_t button)
{
    return buttons[0][button];
}

bool
mpressed (uint8_t button)
{
    return !buttons[1][button] && buttons[0][button];
}

void
mouse_wait (bool signal)
{
    uint_t timeout = 100000;
    if (signal)
        {
            while (timeout--)
                if ((inb (0x64) & 2) == 0)
                    return;
            return;
        }
    else
        {
            while (timeout--)
                if ((inb (0x64) & 1) == 1)
                    return;
            return;
        }
}

uint8_t
mouse_read (void)
{
    mouse_wait (0);
    return inb (0x60);
}

uint8_t
mouse_write (uint8_t x)
{
    outb (0x64, 0xd4);
    mouse_wait (1);
    outb (0x60, x);
    return mouse_read ();
}

void
setup_mouse (void)
{
    uint8_t ccb;

    outb (0x64, 0xa8);

    outb (0x64, 0x20);
    ccb = inb (0x60) | 2;

    outb (0x64, 0x60);
    outb (0x60, ccb);

    mouse_write (0xf6);
    mouse_write (0xf4);
}

void
kernel (void)
{
    clear (0);
    paint ();

    remap_irq ();
    toggle_irq (I_TIMER, true);
    toggle_irq (I_KEYBOARD, true);
    toggle_irq (I_MOUSE, true);
    timer (0, 60);

    setup_mouse ();

    asm ("sti");

    mdraw ();

    forever ();
}
