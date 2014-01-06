#include <stdio.h>

int main(int argc, char **argv) {

	int x;

	printf("\x1b[0;37mTesting 24-bit color mode:\n");

	for(x=0;x<256;x++) {
		if ((x%64==0)) printf("\n");
		printf("\x1b[38;2;%d;0;0m#",x);
	}

	for(x=0;x<256;x++) {
		if ((x%64==0)) printf("\n");
		printf("\x1b[38;2;0;%d;0m#",x);
	}

	for(x=0;x<256;x++) {
		if ((x%64==0)) printf("\n");
		printf("\x1b[38;2;0;0;%dm#",x);
	}

	/* Background */

	for(x=0;x<256;x++) {
		if (x%64==0) printf("\n");
		printf("\x1b[48;2;%d;0;0m ",x);
	}

	for(x=0;x<256;x++) {
		if (x%64==0) printf("\n");
		printf("\x1b[48;2;0;%d;0m ",x);
	}

	for(x=0;x<256;x++) {
		if (x%64==0) printf("\n");
		printf("\x1b[48;2;0;0;%dm ",x);
	}

	printf("\x1b[1;37;40m\n");
	return 0;
}
