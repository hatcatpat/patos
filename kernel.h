#ifndef KERNEL
#define KERNEL

#include "font.h"
#include "math.h"
#include "scancode.h"
#include "x.h"

#define W 320
#define H 200
#define FONTW (W / 4)
#define FONTH (H / 4)

#define asm __asm__

extern char *vga;
extern uint8_t buffer[W * H];
extern int cursor[2];
extern uint8_t color[2];
extern bool keys[2][0xff];
extern bool buttons[2][3];
extern int mouse[2];
extern uint_t ticks, beep_dur;

void kernel (void);

/* misc */
void forever (void);
void halt (void);
void sleep (uint_t dur);
/**/

/* io */
uint8_t inb (uint16_t port);
void outb (uint16_t port, uint8_t data);
void com1 (uint8_t data);
void io_wait (void);
/**/

/* memory */
void copy (void *dest, void *src, uint_t count);
void set (void *dest, int value, uint_t count);
uint_t slen (char *str);
/**/

/* vga */
void clear (uint8_t bg);
void paint (void);
void cursornewline (void);
void cursornext (void);
void cursortab (void);
void putc (char c);
void putn (uint_t n);
void putnh (uint_t n);
void puts (char *str);
void putpixel (int16_t x, int16_t y, uint8_t col);
void putrect (int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t col);
void putellipse (int16_t cx, int16_t cy, uint16_t w, uint16_t h, uint8_t col);
void putbms (uint8_t *bm, uint_t sx, uint_t sy, int16_t x, int16_t y,
             uint8_t w, uint8_t h);
void putbm (uint8_t *bm, int16_t x, int16_t y, uint8_t w, uint8_t h);
/**/

/* interrupts */
void exception (uint8_t e);
void irq (uint8_t e);
void toggle_irq (uint8_t x, bool enable);
void remap_irq (void);
void timer (uint8_t chan, uint32_t hz);
/**/

/* pcspk */
void sound (void);
void nosound (void);
void beep (uint_t freq, uint_t dur);
void nobeep (void);
/**/

/* keyboard */
bool held (uint8_t key);
bool pressed (uint8_t key);
/**/

/* mouse */
void setup_mouse (void);
bool mheld (uint8_t button);
bool mpressed (uint8_t button);
/**/

/* fun */
void bounce (void);
void froggame (void);
void mdraw (void);
/**/

#endif
