#include "LPC11xx.h"

void printf(char* format, ...) {
	__asm volatile ("nop");
}

int main() {
	printf("Hallo wie geht's? %s %i %i %i Test %s", "Blub", 42, 4, 1, "Bla");
}