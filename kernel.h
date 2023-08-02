#include "font.h"
#include "scancode.h"
#include "x.h"

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
uint_t strlen (char *str);
/**/

/* vga */
void clear (char bg);
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
/**/

/* interrupts */
void exception (uint8_t e);
void irq (uint8_t e);
void mask_irq (uint8_t x, bool enable);
void remap_irq (void);
void timer (uint8_t chan, uint32_t hz);
/**/

/* pcspk */
void sound ();
void nosound (void);
void beep (uint_t freq, uint_t dur);
void nobeep ();
/**/

/* fun */
void bounce (void);
/**/
