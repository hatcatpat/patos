cross=/home/pat/opt/cross/bin
cc=$(cross)/i686-elf-gcc
ld=$(cross)/i686-elf-ld
size=$(cross)/i686-elf-size
objdump=$(cross)/i686-elf-objdump -M intel -d
gdb=$(cross)/i686-elf-gdb

all: os.img

boot.bin: boot.asm
	nasm boot.asm -o boot.bin

entry.o: entry.asm
	nasm -f elf entry.asm -o entry.o

cflags=-O0 -g -ffreestanding -fno-builtin -nostdlib -nostdinc -Wall -ansi -masm=intel -m32
kernel.o: *.c *.h
	$(cc) $(cflags) -c kernel.c -o kernel.o

kernel.bin: kernel.o entry.o linker.ld
	$(ld) -T linker.ld entry.o kernel.o -o kernel.bin

os.img: boot.bin kernel.bin
	cat boot.bin kernel.bin > os.img

clean:
	rm *.o *.bin *.img *.log *.out

qemuflags=-no-reboot \
		  -serial file:serial.log \
		  -drive file=os.img,format=raw,index=0,media=disk \
		  -audiodev alsa,id=speaker -machine pcspk-audiodev=speaker
		  # -audiodev jack,id=speaker,out.connect-ports=playback_* -machine pcspk-audiodev=speaker
run: all
	qemu-system-i386 $(qemuflags)

log: all
	qemu-system-i386 $(qemuflags) -D qemu.log

gdb: all
	qemu-system-i386 $(qemuflags) -s -S &
	$(gdb)

ld: all
	$(ld) -T linker.ld entry.o kernel.o -Map ld.log
	rm a.out

objdump: all
	echo '[[entry]]' > objdump.log
	$(objdump) entry.o >> objdump.log
	echo '[[kernel]]' >> objdump.log
	$(objdump) kernel.o >> objdump.log
	cat objdump.log

size: all
	$(size) entry.o > size.log
	$(size) kernel.o >> size.log
	cat size.log

logs: log ld objdump size
	vim *.log -p
