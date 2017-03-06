mbr : mbr.S
	gcc -m32 -c mbr.S
	ld -m elf_i386 -e start -Ttext=0x7C00 -o mbr mbr.o
	objcopy --strip-all --only-section=.text --output-target=binary mbr mbr
	./make-mbr mbr

clean :
	rm mbr mbr.o
