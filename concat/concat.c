
#include <stdio.h>
#include <string.h>
#include <ctype.h>

int main(int argc, char *argv[])
{
	char buf[1024];
	int output = 0;
	char *at;
	FILE *infile1;
	FILE *infile2;
	FILE *outfile;

	if (argc!=4) {
		printf("Wrong number of arguments\n");
		return 1;
	}
	infile1 = fopen(argv[1],"r");
	if (infile1 == NULL) {
		printf("Couldn't open %s\n",argv[1]);
		return 1;
	}
	infile2 = fopen(argv[2],"r");
	if (infile2 == NULL) {
		printf("Couldn't open %s\n",argv[2]);
		return 1;
	}
	outfile = fopen(argv[3],"w");
	if (outfile == NULL) {
		printf("Couldn't open %s\n",argv[3]);
		return 1;
	}

	while (fgets(buf,1024,infile1)!=NULL) {
		if (memcmp(buf,"0 @G",4) == 0) {
			while (fgets(buf,1024,infile2)!=NULL) {
				if (memcmp(buf,"0 ",2) == 0) {
					if (buf[3] == 'L') break;
					if (isdigit(buf[3])) output = 1; else output = 0;
				}
				at = strchr(buf,'@');
				if (at) {
					memmove(at+2,at+1,1000-(at-buf));
					at[1] = 'J';
				}
				if (output) fprintf(outfile,"%s",buf);
			}
			fprintf(outfile,"0 @G1@ _GRAPHICS\n");
		} else if (memcmp(buf,"0 TRLR",6) == 0) {
			while (fgets(buf,1024,infile2)!=NULL) {
				at = strchr(buf,'@');
				if (at) {
					memmove(at+2,at+1,1000-(at-buf));
					at[1] = 'J';
				}
				fprintf(outfile,"%s",buf);
			}
		} else {
			fprintf(outfile,"%s",buf);
		}
	}




	return 0;
}

