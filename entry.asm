bits 32
section .text

KERNEL_OFFSET equ 0x10000

extern kernel
extern exception
extern irq

global entry
entry:
    mov esp, stack
	lidt [idt_end]
	call kernel

%macro IDT_ENTRY 1
dw (KERNEL_OFFSET + %1 - $$) & 0xffff ; offset low
dw 1_0_00b ; segment
db 0 ; zero
db 0x8e ; flags
dw (KERNEL_OFFSET + %1 - $$) >> 16 ; offset high
%endmacro

idt_start:
	%assign i 0
	%rep 32
		IDT_ENTRY isr%+ i
	%assign i i + 1
	%endrep

	; IDT_ENTRY irq_timer

	%assign i 0
	%rep 16
		IDT_ENTRY irq%+ i
	%assign i i + 1
	%endrep
idt_end:
    dw idt_end - idt_start - 1
	dd idt_start

%macro ISR 1
isr%+ %1:
	cli
	push byte %1
	call exception
%endmacro

%assign i 0
%rep 32
	ISR i
%assign i i + 1
%endrep

%macro IRQ 1
irq%+ %1:
	cld
	pusha
	push byte %1
	call irq
	add esp, 4
	popa
	iret
%endmacro

%assign i 0
%rep 16
	IRQ i
%assign i i + 1
%endrep

extern ticks
irq_timer:
	pusha
	add word [ticks], 1
	mov ax, 0x20
	mov dx, ax
    out dx, ax
	popa
	iret

section .bss
resb 0x1000
stack:
