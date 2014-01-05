#include <stdio.h>

int main(int argc, char **argv) {

	int x,y;

	printf("\x1b[1;37mTesting plain 16x8 colors:\n");

	for(y=0;y<8;y++) {
		for(x=0;x<16;x++) {
			printf("\x1b[%d;3%d;4%dm#",x>=8,x%8,y%8);
		}
		printf("\x1b[1;37;40m\n");
	}

	return 0;
}
