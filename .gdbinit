set disassembly-flavor intel
add-symbol-file kernel.o 0x10000
target remote :1234
