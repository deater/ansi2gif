#include <stdio.h>
#include <ctype.h>

int main(int argc,char **argv) {
   
    FILE *fff;
    unsigned char buff[16];
    int j,i;
   
    if (argc<1) {
       printf("Usage: font2include fontfilename\n\n");
       exit(5);
    }
    if ((fff=fopen(argv[1],"r"))==NULL) {
       printf("Error!  %s does not exist.\n\n",argv[1]);
       exit(7);
    }
    printf ("unsigned char default_font[256][16] = { \n");
    j=0;
    while ((j<256) && (!feof(fff))) {
       j++;
       fread(buff,1,16,fff);
       if (isprint(j)) printf("/* '%c' */",j);
       printf("\t{ ");
       for(i=0;i<15;i++) {
	  printf("'\\%o'",buff[i]);
	  if(i<14) printf(",");
	  if(i==8) printf("\n\t\t");
       }
       printf("}");
       if (j<256) printf(",");
       printf("\n");
    }
    printf("};\n");
    fclose(fff);
    return 0;
}
