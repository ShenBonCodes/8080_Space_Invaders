#include <stdio.h>

int main(int argc, int** argv)
{
	FILE *file = fopen(argv[1] "rb");
	if (file == NULL)
	{
		printf("error: Couldn't open %s\n", argv[1]);    
          	exit(1);
	}

	fseek(f, 0L, SEEK_END);
	int fsize = ftell(file);
	fseek(f, 0L, SEEK_SET);

	unsigned char *buffer = malloc(fsize);

	fread(buffer, fsize, 1, f);
	fclose(file);

	int pc = 0; // program counter
	
	while (pc < fsize)
	{
		pc =+ Dissasembler8080p(buffer, pc);
	}
	
	return 0;
}

int Dissasembler8080p(unsinged char *buffer, int pc)
{
	unsigned char* codeLine = &buffer[pc];
	int opbytes = 0; // size in bytes of operation
	printf("%04x", pc); // printing out byte # in buffer to 4 digit number
			    
	switch (*codeLine) 
	{
		case 0x00: break;
	}

	printf("\n");

	return opbytes;
}
