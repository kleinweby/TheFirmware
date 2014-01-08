simulator: simulator.o
	clang++ -ggdb -fblocks -o simulator simulator.o ${LLVM_LDFLAGS}

simulator.o: simulator.c
	clang -ggdb -fblocks -c -o simulator.o simulator.c ${LLVM_CFLAGS}
