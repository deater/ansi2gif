#include <stdio.h>

int main(int argc, char **argv) {

	int x;

	printf("\x1b[0;37mTesting 256 color mode:\n");

	for(x=0;x<256;x++) {
		if (x==16) printf("\n");
		if ((x>16) && ((x-16)%12==0)) printf("\n");
		printf("\x1b[38;5;%dm#",x);
	}
	printf("\x1b[1;37;40m\n");
	return 0;
}
