bits 16
org 0x7c00

boot:
	cli

	; load kernel
	mov ax, KERNEL_OFFSET / 16
	mov es, ax ; target sector
	mov al, 32 ; sectors to read
	mov ah, 2 ; bios read sector function
    mov bx, 0 ; target offset
	mov cl, 2 ; sector = boot sector + 1 = 2
	mov ch, 0 ; cylinder to read
	mov dh, 0 ; head number
	int 0x13

	mov ax, 0x13
	int 0x10 ; set vga mode

	mov ax, 0x2401
	int 0x15 ; a20 mode

	lgdt [gdt_end]

	mov eax, cr0
	or eax, 1 ; enable protected mode bit
	mov cr0, eax
	jmp CODE_SEG:protected

gdt_start:
	dq 0 ; null segment
gdt_code:
	dw 0xffff
	dw 0
	db 0
	db 0x9a
	db 0xcf
	db 0
gdt_data:
	dw 0xffff
	dw 0
	db 0
	db 0x92
	db 0xcf
	db 0
gdt_end:
    dw gdt_end - gdt_start - 1
	dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

bits 32

protected:
	; reset registers
	mov ax, DATA_SEG
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	call KERNEL_OFFSET

KERNEL_OFFSET equ 0x10000

times 510 - ($ - $$) db 0
dw 0xaa55
