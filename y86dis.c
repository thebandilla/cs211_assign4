#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include "y86emul.h"
#include <math.h>
#include <stdlib.h>

const char *reg[8] = {"%eax", "%ecx", "%edx", "%ebx", "%esp", "%ebp", "%esi", "%edi"};

/*
	Strings representing the register corresponding to its associated encoding.
*/

int pc; 

/*
	Similar to the pc in the emulator, but will only be used to step through
	the string and print out instructions.
*/


int main (int argc, char ** argv)
{
	//	Checks for the correct number of arguements

	if (argc <= 1)
	{
		printf("ERROR: Not enough input arguements!\n");
		return 0;
	}

	//	Checks for the help flag and prints the usage of this program
	
	char helptest[] = "-h\0";
	char * input = (char *)malloc(sizeof(argv[1]));
	strcpy(input, argv[1]);
	
	if (strcmp(input, helptest) == 0)
	{
		printf("This emulator can be used to run programs written in Y86 instructions.\n");
		printf("Usage: \n");
		printf("./y86emul <y86 file name>\n");
		free(input);
		return 0;
	}

	//	Checks the arguement to see if it has a correct file extension
	
	if (strlen(input) < 5)
	{
		printf("ERROR: Invalid input file: %s\n", input);	
		free(input);
		return 0;
	}
//	Finds the start of the file extension
	
	int i = 0;
	int j = 0;

	
	for (; input[i] != '\0'; i++)
	{
		if (input[i] == '.')
		{
			break;
		}
	}
	
	char fextention[] = ".y86";
	char * temp = &input[i];
	
	if (strcmp(temp, fextention) != 0)
	{
		printf("ERROR: Invalid file extension: %s\n", temp);
		printf("This program only accepts .y86 files.\n");
		return 0;
	}

	char mode[] = "r\0";
	FILE * inputfile = fopen(input, mode);
	if (inputfile == NULL)
	{
		printf("ERROR: File not found: %s\n", input);
		printf("The file must be in the same directory as the executeable.\n");
		return 0;
	}

	char c;
	char * prog = (char *) calloc(1, sizeof(char));
	prog[0] = '\0';
	
	do
	{
		c = fgetc(inputfile);
//		printf("%c", c);
		if (feof(inputfile))
		{
			break;
		}
		
		prog = append(prog, c);
		
	} while(1);

	fclose(inputfile);
//	for (i = 0; i < strlen(prog); i++)
//	{
//		printf("%c", prog[i]);
//	}
	char * duplicate = copy(prog);
	char * token = strtok(duplicate,"\n\t\r");
	char * instructions;
	char * pcstart;
	int count = 0;

	do
	{
//		printf("%s\n", token);
		if (strcmp(token, ".text") == 0)
		{
			if (count == 0)
			{	
				count++;
			}
			else
			{
				printf("ERROR:\n\t More than one .text directive has been detected. \n");
				printf("\t Please make sure that the file has exactly one .text directive \n");
				return 0;
			}
			
			pcstart = copy(strtok (NULL, "\n\t\r"));
			instructions = copy(strtok(NULL, "\n\t\r"));	
		}
		
		token = strtok(NULL, "\n\t\r");		//	Continue interating through the tokens
		
		if (token == NULL)
		{
			break;				//	Exit when we get to the last token
		}
	} while (1);
//	printf("%x\n%s\n",hextodec(pcstart),instructions);
	free(duplicate);

	pc = hextodec(pcstart);

//	printf("%s\n", instructions);

	int is = strlen(instructions);
//	printf("%d\n", is);

	unsigned char memspace[is/2];

	i = j = 0;
	while (j < is)
	{
		memspace[i] = gettwobytes(instructions, j);
//		printf("%x ", gettwobytes(instructions, j));
		i++;
		j += 2;
	}

	for(i = 0; i < is/2; i++)
	{
//		printf("%x ",memspace[i]);
	}
//	printf("\n");

	union converter con;

	int arg1, arg2, value;

	i = j = 0;
	while (i < is/2)
	{
		switch (memspace[i])
		{
			case 0x00:
				printf("[0x%08x]\tnop\n", pc);
				i++;
				j = 1;
			break;

			case 0x10:
				printf("[0x%08x]\thlt\n", pc);
				i++;
				j = 1;
			break;
			
			case 0x20:
				
				arg1 = (memspace[i + 1] & 0xf0) >> 4;
				arg2 = (memspace[i + 1] & 0x0f);

				printf("[0x%08x]\trrmovl\t%s\t%s\n", pc, reg[arg1], reg[arg2]);
				j = 2;
				i += 2;

			break;
			
			case 0x30:
				
				arg1 = (memspace[i + 1] & 0xf0) >> 4;
				arg2 = (memspace[i + 1] & 0x0f);
				
				con.byte[0] = memspace[i + 2];
				con.byte[1] = memspace[i + 3];
				con.byte[2] = memspace[i + 4];
				con.byte[3] = memspace[i + 5];
				
				printf("[0x%08x]\tirmovl\t$%0x\t%s\n", pc, con.integer, reg[arg2]);
				j = 6;
				i += 6;			
				
				break;
			break;

			case 0x40:
				
				arg1 = (memspace[i + 1] & 0xf0) >> 4;
				arg2 = (memspace[i + 1] & 0x0f);
				
				con.byte[0] = memspace[i + 2];
				con.byte[1] = memspace[i + 3];
				con.byte[2] = memspace[i + 4];
				con.byte[3] = memspace[i + 5];

				value = con.integer;
				
				printf("[0x%08x]\trmmovl\t%s\t%d%s\n", pc, reg[arg1], value, reg[arg2]);

				j = 6;
				i += 6;

			break;
			
			case 0x50:

				arg1 = (memspace[i + 1] & 0xf0) >> 4;
				arg2 = (memspace[i + 1] & 0x0f);

				con.byte[0] = memspace[i + 2];
				con.byte[1] = memspace[i + 3];
				con.byte[2] = memspace[i + 4];
				con.byte[3] = memspace[i + 5];

				value = con.integer;

				printf("[0x%08x]\tmrmovl\t%d%s\t%s\n", pc, value, reg[arg2], reg[arg1]);

				j = 6;
				i += 6;

			break;
			
			case 0x60:

				arg1 = (memspace[i + 1] & 0xf0) >> 4;
				arg2 = (memspace[i + 1] & 0x0f);

				printf("[0x%08x]\taddl\t%s\t%s\n", pc, reg[arg1], reg[arg2]);
				j = 2;
				i += 2;

			break;
			
			case 0x61:

				arg1 = (memspace[i + 1] & 0xf0) >> 4;
				arg2 = (memspace[i + 1] & 0x0f);

				printf("[0x%08x]\tsubl\t%s\t%s\n", pc, reg[arg1], reg[arg2]);
				
				j = 2;
				i += 2;

			break;
			
			case 0x62:

				arg1 = (memspace[i + 1] & 0xf0) >> 4;
				arg2 = (memspace[i + 1] & 0x0f);

				printf("[0x%08x]\tandl\t%s\t%s\n", pc, reg[arg1], reg[arg2]);
				
				j = 2;
				i += 2;

			break;
			
			case 0x63:

				arg1 = (memspace[i + 1] & 0xf0) >> 4;
				arg2 = (memspace[i + 1] & 0x0f);

				printf("[0x%08x]\txorl\t%s\t%s\n", pc, reg[arg1], reg[arg2]);
				
				j = 2;
				i += 2;

			break;
			
			case 0x64:
				
				arg1 = (memspace[i + 1] & 0xf0) >> 4;
				arg2 = (memspace[i + 1] & 0x0f);

				printf("[0x%08x]\tmull\t%s\t%s\n", pc, reg[arg1], reg[arg2]);
				
				j = 2;
				i += 2;

			break;
			
			case 0x65:

				arg1 = (memspace[i + 1] & 0xf0) >> 4;
				arg2 = (memspace[i + 1] & 0x0f);

				printf("[0x%08x]\tcmpl\t%s\t%s\n", pc, reg[arg1], reg[arg2]);
				
				j = 2;
				i += 2;

			break;
			
			case 0x70:

				con.byte[0] = memspace[i + 1];
				con.byte[1] = memspace[i + 2];
				con.byte[2] = memspace[i + 3];
				con.byte[3] = memspace[i + 4];

				value = con.integer;

				printf("[0x%08x]\tjmp\t$0x%x\n", pc, value);

				j = 5;
				i += 5;

			break;
			
			case 0x71:

				con.byte[0] = memspace[i + 1];
				con.byte[1] = memspace[i + 2];
				con.byte[2] = memspace[i + 3];
				con.byte[3] = memspace[i + 4];

				value = con.integer;

				printf("[0x%08x]\tjle\t$0x%x\n", pc, value);

				j = 5;
				i += 5;


			break;
			
			case 0x72:

				con.byte[0] = memspace[i + 1];
				con.byte[1] = memspace[i + 2];
				con.byte[2] = memspace[i + 3];
				con.byte[3] = memspace[i + 4];

				value = con.integer;

				printf("[0x%08x]\tjl\t$0x%x\n", pc, value);

				j = 5;
				i += 5;

			break;
			
			case 0x73:
				
				con.byte[0] = memspace[i + 1];
				con.byte[1] = memspace[i + 2];
				con.byte[2] = memspace[i + 3];
				con.byte[3] = memspace[i + 4];

				value = con.integer;

				printf("[0x%08x]\tje\t$0x%x\n", pc, value);

				j = 5;
				i += 5;

			break;
			
			case 0x74:
				
				con.byte[0] = memspace[i + 1];
				con.byte[1] = memspace[i + 2];
				con.byte[2] = memspace[i + 3];
				con.byte[3] = memspace[i + 4];

				value = con.integer;

				printf("[0x%08x]\tjne\t$0x%x\n", pc, value);

				j = 5;
				i += 5;

			break;
			
			case 0x75:
				
				con.byte[0] = memspace[i + 1];
				con.byte[1] = memspace[i + 2];
				con.byte[2] = memspace[i + 3];
				con.byte[3] = memspace[i + 4];

				value = con.integer;

				printf("[0x%08x]\tjge\t$0x%x\n", pc, value);

				j = 5;
				i += 5;

			break;
			
			case 0x76:
				
				con.byte[0] = memspace[i + 1];
				con.byte[1] = memspace[i + 2];
				con.byte[2] = memspace[i + 3];
				con.byte[3] = memspace[i + 4];

				value = con.integer;

				printf("[0x%08x]\tjg\t$0x%x\n", pc, value);

				j = 5;
				i += 5;

			break;
			
			case 0x80:

				con.byte[0] = memspace[i + 1];
				con.byte[1] = memspace[i + 2];
				con.byte[2] = memspace[i + 3];
				con.byte[3] = memspace[i + 4];

				value = con.integer;

				printf("[0x%08x]\tcall\t$0x%x\n", pc, value);

				j = 5;
				i += 5;

			break;
			
			case 0x90:
				
				printf("[0x%08x]\tret\n", pc);
				j = 5;
				i += 5;

			break;
			
			case 0xA0:

				arg1 = (memspace[i + 1] & 0xf0) >> 4;

				printf("[0x%08x]\tpushl\t%s\n", pc, reg[arg1]);

				j = 2;
				i += 2;

			break;
			
			case 0xB0:

				arg1 = (memspace[i + 1] & 0xf0) >> 4;

				printf("[0x%08x]\tpopl\t%s\n", pc, reg[arg1]);

				j = 2;
				i += 2;

			break;
			
			case 0xC0:

				arg1 = (memspace[i + 1] & 0xf0) >> 4;
				con.byte[0] = memspace[i + 2];
				con.byte[1] = memspace[i + 3];
				con.byte[2] = memspace[i + 4];
				con.byte[3] = memspace[i + 5];

				value = con.integer;

				printf("[0x%08x]\treadb\t%d%s\n", pc, value, reg[arg1]);

				j = 6;
				i += 6;

			break;
			
			case 0xC1:
				
				arg1 = (memspace[i + 1] & 0xf0) >> 4;
				con.byte[0] = memspace[i + 2];
				con.byte[1] = memspace[i + 3];
				con.byte[2] = memspace[i + 4];
				con.byte[3] = memspace[i + 5];

				value = con.integer;

				printf("[0x%08x]\treadl\t%d%s\n", pc, value, reg[arg1]);

				j = 6;
				i += 6;

			break;
			
			case 0xD0:
				
				arg1 = (memspace[i + 1] & 0xf0) >> 4;
				con.byte[0] = memspace[i + 2];
				con.byte[1] = memspace[i + 3];
				con.byte[2] = memspace[i + 4];
				con.byte[3] = memspace[i + 5];

				value = con.integer;

				printf("[0x%08x]\twriteb\t%d%s\n", pc, value, reg[arg1]);

				j = 6;
				i += 6;

			break;
			
			case 0xD1:
				
				arg1 = (memspace[i + 1] & 0xf0) >> 4;
				con.byte[0] = memspace[i + 2];
				con.byte[1] = memspace[i + 3];
				con.byte[2] = memspace[i + 4];
				con.byte[3] = memspace[i + 5];

				value = con.integer;

				printf("[0x%08x]\twritel\t%d%s\n", pc, value, reg[arg1]);

				j = 6;
				i += 6;

			break;
			
			case 0xE0:

				arg1 = (memspace[i + 1] & 0xf0) >> 4;
				arg2 = (memspace[i + 1] & 0x0f);

				con.byte[0] = memspace[i + 2];
				con.byte[1] = memspace[i + 3];
				con.byte[2] = memspace[i + 4];
				con.byte[3] = memspace[i + 5];

				value = con.integer;

				printf("[0x%08x]\tmovsbl\t%d%s\t%s\n", pc, value, reg[arg2], reg[arg1]);

				j = 6;
				i += 6;


			break;
			
			default:

			break;
			

		}
		pc += j;
	}






	free(input);	
	free(prog);
	return 0;
}

/*
	Appends a character to the end of a string
*/

char * append (char * str, char c)
{
	int len = strlen(str) + 2;
	char * ret = (char *)calloc(len, sizeof(char));
	strcpy(ret, str);
	free(str);
	ret[len-1] = '\0';
	ret[len-2] = c;
	return ret;
}
	

/*
	Converts hex strings to an integer output using the following
	two functions.
*/

int hextodec(char * num)
{
	// First convert hex string to binary string
	int size = strlen(num);
	char * binstr = (char *) malloc((4*size + 1)*sizeof(char));
	int i;
	for (i = 0; i < 4*size + 1; i++)
	{
		binstr[i] = '\0';
	}
	for (i = 0; i < size; i++)
	{
		strcat(binstr, hextobin(num[i]));
	}
	// Converts from binary to decimal
	int ret = bintodec(binstr);
	free(binstr);
	return ret;
}
	
/*
	Converts a single hex character to the equivalent binary string
*/
char * hextobin(char c) 
{
	switch(c)
	{
		case '0':
		return "0000";
		break;
		
		case '1':
		return "0001";
		break;
		
		case '2':
		return "0010";
		break;
		
		case '3':
		return "0011";
		break;
		
		case '4':
		return "0100";
		break;
		
		case '5':
		return "0101";
		break;
		
		case '6':
		return "0110";
		break;
		
		case '7':
		return "0111";
		break;
		
		case '8':
		return "1000";
		break;
		
		case '9':
		return "1001";
		break;
		
		case 'a':
		case 'A':
		return "1010";
		break;
		
		case 'b':
		case 'B':
		return "1011";
		break;
		
		case 'c':
		case 'C':
		return "1100";
		break;
		
		case 'd':
		case 'D':
		return "1101";
		break;
		
		case 'e':
		case 'E':
		return "1110";
		break;
		
		case 'f':
		case 'F':
		return "1111";
		break;
		
		case '\0':
		break;
		
		default:
		printf("Invalid hex character: %c \n", c);
		break;
	}
	return "";
}
	
/*
	Converts binary string to a decimal output
*/

int bintodec(char * num)
{
	int power = strlen(num) - 1;
	int i, ret = 0;
	for (i = 0; num[i] != '\0'; i++)
	{
		int temp = num[i] - '0';
		ret += temp * (int)pow(2, power);
		power--;
	}
	return ret;
}

/*
	Creates a copy of the input string and returns a pointer 
	to the new string.
*/

char * copy (char * str)
{
	char * ret = (char *) malloc((strlen(str) + 1) * sizeof(char));
	strcpy(ret, str);
	return ret;
}

/*
	Gets *one* byte but two characters. 
	Technically it is two bytes, but because we can assume good input, 
	it is one byte in unsigned char
*/

int gettwobytes(char * str, int position)
{
	char * twobytes = (char *) malloc(3*sizeof(char));
	
	twobytes[2] = '\0';
	twobytes[0] = str[position];
	twobytes[1] = str[position + 1];
	
	int ret = hextodec(twobytes);
	
	free(twobytes);
	
	return ret;
}
