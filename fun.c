#include "kernel.h"

void
bounce (void)
{
    static int x = FONTW / 2, y = FONTH / 2, dx = 0, dy = 0;
    static char *hello = "hello";
    /* sound (); */
    for (;;)
        {
            clear (keys[0][1] ? 8 : 4);
            cursor[0] = x, cursor[1] = y;
            puts (hello);
            cursor[0] = cursor[1] = 0;
            putn (ticks);
            paint ();

            if (dx == 0 && x == 0)
                {
                    dx = 1;
                    beep (440, 10);
                }
            else if (dx == 1 && x == FONTW - slen (hello) + 1)
                {
                    dx = 0;
                    beep (220, 10);
                }

            if (dx == 0)
                x--;
            else if (dx == 1)
                x++;

            if (dy == 0 && y == 0)
                {
                    dy = 1;
                    beep (660, 10);
                }
            else if (dy == 1 && y == FONTH - 1)
                {
                    dy = 0;
                    beep (770, 10);
                }

            if (dy == 0)
                y--;
            else if (dy == 1)
                y++;

            sleep (1);
        }
}

void
froggame (void)
{
    float x = 0, y = 0, vx = 0, vy = 0, spd = 0.1;
    uint8_t jumping = 2, score = 0;
    int bx = random () % W, by = random () % H;
    int musicp = 0, musict = 0;
#define MUSIC_LEN 32
    uint8_t music[MUSIC_LEN] = {
        0, 0,  4, 2, /**/ 3, 4, 4, 0, /**/
        0, 9,  0, 2, /**/ 0, 2, 9, 0, /**/
        4, 0,  4, 2, /**/ 3, 4, 4, 0, /**/
        0, 10, 0, 0, /**/ 5, 0, 4, 2, /**/
    };

#define R 40
#define G 47
#define B 32
    uint8_t frog[4 * 4] = {
        G, 0, 0, G, /**/
        B, G, G, B, /**/
        G, R, R, G, /**/
        G, G, G, G, /**/
    };

    uint8_t berry[4 * 4] = {
        0, G, G, 0, /**/
        R, G, R, 0, /**/
        R, R, R, 0, /**/
        R, R, R, 0, /**/
    };
#undef R
#undef G
#undef B

#define DAMP_X 0.99
#define GRAV 0.3
#define JUMP 64
    sound ();
    font_scale = 4;

    for (;;)
        {
            if (musict++ % 8 == 0)
                {
                    uint_t note = music[musicp];
                    musicp = (musicp + 1) % MUSIC_LEN;
                    if (note != 0)
                        beep (440 + note * 110, 10);
                }

            if (held (char2scancode ('d')))
                vx += spd;
            else if (held (char2scancode ('a')))
                vx -= spd;

            x += vx, y += vy;

            if (x < 0)
                {
                    x = 0, vx *= -1;
                    if (jumping)
                        jumping--;
                }
            else if (W - 8 * 4 <= x)
                {
                    x = W - 8 * 4 - 1, vx *= -1;
                    if (jumping)
                        jumping--;
                }

            if (y < H - 8 * 4)
                {
                    vy += GRAV;
                    if (vy > 0 && y >= H - 8 * 4 - 8)
                        jumping = 0;
                }
            else
                {
                    jumping = 0;
                    vy = 0, y = H - 8 * 4;
                }

            if ((jumping == 0 && held (char2scancode ('w')))
                || (jumping == 1 && pressed (char2scancode ('w'))))
                {
                    jumping++;
                    vy = -spd * JUMP;
                    beep (110, 1);
                }

            vx *= DAMP_X;

            if (bx <= x + 8 * 4 && x <= bx + 4 * 3)
                if (by <= y + 8 * 4 && y <= by + 4 * 3)
                    {
                        beep (880, 1);
                        bx = random () % (W - 4 * 3);
                        by = random () % (H - 4 * 3);
                        score++;
                    }

            clear (1);
            putrect (0, H - 10, W, 10, 2);
            putimgs (frog, 8, 8, x, y, 4, 4);
            putimgs (berry, 3, 3, bx, by, 4, 4);
            cursor[0] = cursor[1] = 1, putn (score);

            paint ();
            copy (keys[1], keys[0], 0xff);
            sleep (2);
        }
#undef DAMP_X
#undef GRAV
#undef JUMP
}

void
mdraw (void)
{
    static uint8_t canvas[W * H];
    uint8_t pointer[4 * 4] = {
        0,  15, 15, 15, /**/
        0,  0,  15, 15, /**/
        0,  15, 0,  15, /**/
        15, 15, 0,  0,  /**/
    };
    uint8_t sz = 4, col = COLOR_RED;

    set (canvas, color[0], W * H);
    for (;;)
        {
            if (pressed (char2scancode ('r')) || mpressed (2))
                {
                    set (canvas, color[0], W * H);
                    mouse[0] = W / 2, mouse[1] = H / 2;
                }

            if (pressed (char2scancode ('1')))
                col = COLOR_RED;
            else if (pressed (char2scancode ('2')))
                col = COLOR_BLUE;
            else if (pressed (char2scancode ('3')))
                col = COLOR_GREEN;

            if (held (char2scancode ('a')) && sz >= 2)
                sz--;
            else if (held (char2scancode ('d')))
                sz++;

            copy (buffer, canvas, W * H);

            cursor[0] = cursor[1] = 0;
            putrect (0, 0, 4 * 16, 4 * 2, color[0]);
            putn (mouse[0]), putc ('\n'), putn (mouse[1]);

            putimg (pointer, CLAMP (mouse[0], 0, W - 2),
                    CLAMP (mouse[1], 0, H - 2), 4, 4);

            putemptyrect (CLAMP (mouse[0] + 4, 0, W),
                          CLAMP (mouse[1] - sz, 0, H), sz, sz, col);

            if (mheld (0))
                {
                    int i, x, y;
                    x = CLAMP (mouse[0] + 4, 0, W);
                    y = CLAMP (mouse[1] - sz, 0, H);
                    for (i = 0; i < sz * sz; ++i)
                        canvas[x + i % sz + W * (y + i / sz)] = col;
                }

            paint ();

            copy (keys[1], keys[0], 0xff);
            copy (buttons[1], buttons[0], 3);

            sleep (2);
        }
}
