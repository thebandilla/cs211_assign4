// Ryan Bandilla
// Y86 Emulator
// BKR Comp Arch
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include "y86emul.h"
#include <math.h>
#include <stdlib.h>

int reg[8];

/*
 *	Registers to be used by the program
 *	reg[0] == %eax
 *	reg[1] == %ecx
 *	reg[2] == %edx
 *	reg[3] == %ebx
 *	reg[4] == %esp
 *	reg[5] == %ebp
 *	reg[6] == %esi
 *	reg[7] == %edi
 */

int pc;

/*
 *	Program counter
 *	Points to memory where the instruction being executed exists
 *	Will be initialized as a part of processing the .text directive
 *	Will be used as an offset from the starting location of the instructions in memory.
 */

unsigned char * memspace;

/*
 *	Contiguous block of memory where all instructions and data for program 
 *	exectution will be stored.
 *	Will be initialiezed as a part of processing the .size directive
 */

 int memsize;

 /*
  * Stores the length of the chunk of memory
  */

int OF, ZF, SF;

/*
 *	DEFAULT STATE is 0
 *	ACTIVATED STATE is 1
 *	OF Overflow flag
 *	ZF Zero Flag
 *	SF Negative Flag
 */

ProgramStatus status = AOK;

/*
 *	Current status of program execution.
 *	Default is AOK and if no errors are found during exectution program status will remain AOK
 *
 *	Possible States:
 *
 *	AOK - No errors detected from start to current position
 *	HLT - Halt program execution
 *	ADR - Program encountered an invalid or bad address
 *	INS - Invalid instruction encountered
 *	
 *	If the program enters either the HLT, ADR, or INS state
 *	execution will cease and the program will exit
 */

int main (int argc, char ** argv)
{
//	Checks for correct number of arguements
	
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

//	Gets the file after verified it is a correct *.y86 file	

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
		
		if (feof(inputfile))
		{
			break;
		}
		
		prog = append(prog, c);
		
	} while(1);

	fclose(inputfile);
	
	//	Begin processing the file
	//	Taking contents of file and storing it in a string
	//	I don't like using File IO

	char * duplicate = copy(prog);
	char * token = strtok(duplicate,"\n\t\r");
	char * memory;
	int count = 0;
	// First must find the .size directive
	
	do
	{
		if (strcmp(token, ".size") == 0)
		{
			if (count == 0)
			{	
				count++;
			}
			else
			{
				printf("ERROR:\n\t More than one .size directive has been detected. \n");
				printf("\t Please make sure that the file has exactly one .size directive \n");
				return 0;
			}
			memory = strtok(NULL, "\n\t\r");	
		}
		token = strtok(NULL, "\n\t\r");		//	Continue interating through the tokens
		
		if (token == NULL)
		{
			break;				//	Exit when we get to the last token
		}
	} while (1);			

	if (count == 0)				//	Check to make sure there was a .size direvtive found in the file
	{
		printf("ERROR:\n\t No .size directive was detected in the .y86 file. \n\t Please make sure that the file has exactly one .size directive\n");
		return 0;	
	}
	
	// Intialize emulators memory space
	
	int size = hextodec(memory);
	
	memsize = size;
	memspace = (unsigned char *) malloc((size + 1) * sizeof(unsigned char));
	
	pc = -1;	
	
	for (i = 0; i < size; i++)
	{
		memspace[i] = 0;
	}
	
	// Begin looking for the other directives
	free(duplicate);
	
	duplicate = copy(prog);
	token = strtok(duplicate, "\n\t\r");
	
	char * arg;		//	Arguement of the directive
	char * address;		//	Location in memory where the arg is to be stored
	
	int ai = 0;		//	AddressIndex dec representation of hex address in memory
	
	do
	{
		if (strcmp(token, ".size") == 0)
		{
				//	Already dealt with size, but we are starting from the beginning
			token = strtok(NULL, "\n\t\r");
		}
		else if (strcmp(token, ".text") == 0)
		{
			
			address = copy(strtok(NULL, "\n\t\r"));
			arg = copy(strtok(NULL, "\n\t\r"));
			ai = hextodec(address);
			
			if (pc == -1)
			{
				pc = ai;
			}
			else if (pc != -1)
			{
				printf("ERROR: More than one .text directive detected\n");
				return 0;
			}
			
			j = 0;
			
			while (j < strlen(arg))
			{
				memspace[ai] = (unsigned char) gettwobytes(arg, j);
				ai++;
				j += 2;
			}
			
			free(address);
			free(arg);	
		}
		else if (strcmp(token, ".byte") == 0)
		{
			address = copy(strtok(NULL, "\n\t\r"));
			arg = copy(strtok(NULL, "\n\t\r"));
			
			memspace[hextodec(address)] = (unsigned char) hextodec(arg);
			
			free(address);
			free(arg);
		}
		else if (strcmp(token, ".long") == 0)
		{
			address = copy(strtok(NULL, "\n\t\r"));
			arg = copy(strtok(NULL, "\n\t\r"));
			
			union converter con;
			con.integer = atoi(arg);
			
			for(i = 0; i < 4; i++)
			{
				memspace[i+hextodec(address)] = con.byte[i];
			}
			
			free(address);
			free(arg);
		}
		else if (strcmp(token, ".string") == 0)
		{
			address = copy(strtok(NULL, "\n\t\r"));
			arg = copy(strtok(NULL, "\n\t\r"));
			
			int len = strlen(arg);
			
			i = hextodec(address);
			
			for(j = 1; j < len - 1; j++)
			{
				memspace[i] = (unsigned char)arg[j];
				i++;
			}
			
			free(arg);
			free(address);
		}
		else if(strcmp(token, ".bss") == 0)
		{
			//	BKR said he was not using the bss directive
		}
		else if (token[0] == '.')
		{
			status = INS;
		}
		token = strtok(NULL, "\n\t");
		if (token == NULL)
		{
			break;
		}
	} while (1);
	
	free(duplicate);
	
	if (status == INS)
	{
		printf("ERROR: Invalid directive encountered: %s\n", token);
		return 0;
	}
	
	// 	Everything loaded into memory, no we execute
//	printmemory(size);

	executeprog();
	
	printmemory(size);

//	printstatus();

	free(memspace);
	free(input);	
	free(prog);	
	return 0;	
}

void executeprog()
{
	unsigned char arg1;
	unsigned char arg2;
	
	int value;			// Used for any integer operations

	int num1, num2;		// Used in addl, subl, mull, 

	union converter con;// Used to convert between unsigned chars and ints

	status = AOK;

	int badscan;

	char inputchar; 	// For read/write b
	int inputword;		// For read/write w

	// Initialize all registers to 0
	reg[7] = reg[6] = reg[5] = reg[4] = reg[3] = reg[2] = reg[1] = reg[0] = 0;

	// Intialize all flags to 0
	OF = ZF = SF = 0;



	while (status == AOK)
	{
		switch (memspace[pc])
		{
			// 00 NOP
			case 0x00:
				
				pc++;	// NOP so program conitnues
				
			break;

			// 10 HALT
			case 0x10:
				
				status = HLT;	// Assumed this occurs at the end of program
				
			break;

			// 20 RRMOVL srcR desR
			case 0x20:
				
				getargs(&arg1, &arg2);
				
				reg[arg2] = reg[arg1];					//	Puts info from source into destination
				
				pc += 2;

			break;

			// 30 IRMOVL notR desR value
			case 0x30:
				
				getargs(&arg1, &arg2);
				
				if (arg1 < 0x08)
				{
					status = ADR;
					printf("ERROR: IRMOVL instruction has two addresses. Memory Location: %x\n", pc);
					break;
				}
				
				con.byte[0] = memspace[pc + 2];			// Getting the value bytes in
				con.byte[1] = memspace[pc + 3];			// little endian order
				con.byte[2] = memspace[pc + 4];			//
				con.byte[3] = memspace[pc + 5];			//
				
				value = con.integer;					// Using union to convert between the two
				
				reg[arg2] = value;						// Storing final value in destination register
				
				pc += 6;

			break;

			// 40 RMMOVL srcR desR value = (32bit displacement off desR)
			case 0x40:
				
				getargs(&arg1, &arg2);

				con.byte[0] = memspace[pc + 2];			// Getting the value bytes in
				con.byte[1] = memspace[pc + 3];			// little endian order
				con.byte[2] = memspace[pc + 4];			//
				con.byte[3] = memspace[pc + 5];			//
				
				value = con.integer;					// This is the offset amount

				con.integer = reg[arg1];

				if ((value + reg[arg2] + 3) > memsize)
				{
					status = ADR;
					printf("ERROR: RMMOVL instruction address offset larger than memory space. Memory Location: %x\n", pc);
				}

				memspace[value + reg[arg2] + 0] = con.byte[0];	//
				memspace[value + reg[arg2] + 1] = con.byte[1];	// Stores the integer at the specified location
				memspace[value + reg[arg2] + 2] = con.byte[2];	// in little endian order
				memspace[value + reg[arg2] + 3] = con.byte[3];	//

				pc += 6;

			break;

			// 50 MRMOVL desR srcR value = (32bit displacement off srcR)
			case 0x50:

				getargs(&arg1, &arg2);

				con.byte[0] = memspace[pc + 2];			// Getting the value bytes in
				con.byte[1] = memspace[pc + 3];			// little endian order
				con.byte[2] = memspace[pc + 4];			//
				con.byte[3] = memspace[pc + 5];			//
				
				value = con.integer;					// This is the offset amount

				if ((value + reg[arg2] + 3) > memsize)
				{
					status = ADR;
					printf("ERROR: MRMOVL instruction address offset larger than memory space. Memory Location: %x\n", pc);
				}

				con.byte[0] = memspace[value + reg[arg2] + 0];	//
				con.byte[1] = memspace[value + reg[arg2] + 1];	// Stores the integer at the specified address
				con.byte[2] = memspace[value + reg[arg2] + 2];	// in the union to be stored in a register
				con.byte[3] = memspace[value + reg[arg2] + 3];	//

				reg[arg1] = con.integer;

				pc += 6;

			break;

			// 60 ADDL srcR desR 
			case 0x60:
				
				ZF = 0;
				SF = 0;
				OF = 0;
				
				getargs(&arg1, &arg2);
				
				num1 = reg[arg1];
				num2 = reg[arg2];
				
				value = num1 + num2;

				if (value == 0)
				{
					ZF = 1;
				}

				if (value < 0)
				{
					SF = 1;
				}

				if ((value > 0 && num1 < 0 && num2 < 0) || (value < 0 && num1 > 0 && num2 > 0))
				{
					OF = 1;
				}

				reg[arg2] = value;

				pc += 2;

			break;

			// 61 SUBL srcR desR
			case 0x61:
				
				ZF = 0;
				SF = 0;
				OF = 0;
				
				getargs(&arg1, &arg2);
				
				num1 = reg[arg1];
				num2 = reg[arg2];
				
				value = num2 - num1;

				if (value == 0)
				{
					ZF = 1;
				}

				if (value < 0)
				{
					SF = 1;
				}

				if ((value > 0 && num1 > 0 && num2 < 0) || (value < 0 && num1 < 0 && num2 > 0))
				{
					OF = 1;
				}
	
				reg[arg2] = value;

				pc += 2;

			break;

			// 62 ANDL srcR desR
			case 0x62:
				
				SF = 0;
				ZF = 0;
				
				getargs(&arg1, &arg2);
				
				num1 = reg[arg1];
				num2 = reg[arg2];
				
				value = num1 & num2;

				reg[arg2] = value;

				if (value == 0)
				{
					ZF = 1;
				}

				if (value < 0)
				{
					SF = 1;
				}

				pc += 2;

			break;

			// 63 XORL srcR desR
			case 0x63:
				
				ZF = 0;
				SF = 0;
				
				getargs(&arg1, &arg2);
				
				num1 = reg[arg1];
				num2 = reg[arg2];
				
				value = num1 ^ num2;

				reg[arg2] = value;

				if (value == 0)
				{
					ZF = 1;
				}

				if (value < 0)
				{
					SF = 1;
				}

				pc += 2;

			break;

			// 64 MULL srcR desR
			case 0x64:

				ZF = 0;
				SF = 0;
				OF = 0;
				
				getargs(&arg1, &arg2);
				
				num1 = reg[arg1];
				num2 = reg[arg2];
				
				value = num1 * num2;

				if (value == 0)
				{
					ZF = 1;
				} 

				if (value < 0)
				{
					SF = 1;
				}

				if ((value < 0 && num1 < 0 && num2 < 0) || 
					(value < 0 && num1 > 0 && num2 > 0) || 
					(value > 0 && num1 < 0 && num2 > 0) || 
					(value > 0 && num1 > 0 && num2 < 0))
				{
					OF = 1;
				}

				reg[arg2] = value;

				pc += 2;

			break;

			// 65 CMPL
			case 0x65:

				ZF = 0;
				SF = 0;
				OF = 0;
				
				getargs(&arg1, &arg2);
				
				num1 = reg[arg1];
				num2 = reg[arg2];
				
				value = num2 - num1;

				if (value == 0)
				{
					ZF = 1;
				}

				if (value < 0)
				{
					SF = 1;
				}

				if ((value > 0 && num1 > 0 && num2 < 0) || (value < 0 && num1 < 0 && num2 > 0))
				{
					OF = 1;
				}

				pc += 2;
			break;
			
			// 70 JMP 32bit destination
			case 0x70:
				// Unconditional Jump
				con.byte[0] = memspace[pc + 1];			// Getting the value bytes in
				con.byte[1] = memspace[pc + 2];			// little endian order
				con.byte[2] = memspace[pc + 3];			//
				con.byte[3] = memspace[pc + 4];			//
				
				value = con.integer;					// This is the offset amount
			
				pc = value;

			break;

			// 71 JLE 32bit destination
			case 0x71:
				// Jump if less than or equal to
				con.byte[0] = memspace[pc + 1];			// Getting the value bytes in
				con.byte[1] = memspace[pc + 2];			// little endian order
				con.byte[2] = memspace[pc + 3];			//
				con.byte[3] = memspace[pc + 4];			//
				
				value = con.integer;					// This is the offset amount

				if (ZF == 1 || (SF ^ OF))
				{
					pc = value;
				}
				else
				{
					pc += 5;
				}

			break;

			// 72 JL  32bit destination
			case 0x72:
				// Jump if strictly less than
				con.byte[0] = memspace[pc + 1];			// Getting the value bytes in
				con.byte[1] = memspace[pc + 2];			// little endian order
				con.byte[2] = memspace[pc + 3];			//
				con.byte[3] = memspace[pc + 4];			//
				
				value = con.integer;					// This is the offset amount

				if (ZF == 0 && (SF ^ OF))
				{
					pc = value;
				}
				else
				{
					pc += 5;
				}

			break;

			// 73 JE  32bit destination
			case 0x73:
				// Jump if equal
				con.byte[0] = memspace[pc + 1];			// Getting the value bytes in
				con.byte[1] = memspace[pc + 2];			// little endian order
				con.byte[2] = memspace[pc + 3];			//
				con.byte[3] = memspace[pc + 4];			//
				
				value = con.integer;					// This is the offset amount

				if (ZF == 1)
				{
					pc = value;
				}
				else
				{
					pc += 5;
				}

			break;

			// 74 JNE 32bit destination
			case 0x74:
				// Jump if not equal
				con.byte[0] = memspace[pc + 1];			// Getting the value bytes in
				con.byte[1] = memspace[pc + 2];			// little endian order
				con.byte[2] = memspace[pc + 3];			//
				con.byte[3] = memspace[pc + 4];			//
				
				value = con.integer;					// This is the offset amouny

				if (ZF == 0)
				{
					pc = value;
				}
				else
				{
					pc += 5;
				}
				
			break;

			// 75 JGE 32bit destination
			case 0x75:
			// Jump if greater than or equal to
				con.byte[0] = memspace[pc + 1];			// Getting the value bytes in
				con.byte[1] = memspace[pc + 2];			// little endian order
				con.byte[2] = memspace[pc + 3];			//
				con.byte[3] = memspace[pc + 4];			//
				
				value = con.integer;					// This is the offset amount

				if (!(ZF == 0 && (SF ^ OF)))
				{
					pc = value;
				}
				else
				{
					pc += 5;
				}

			break;

			// 76 JG  32bit destination
			case 0x76:
			// Jump if strictly greater than
				con.byte[0] = memspace[pc + 1];			// Getting the value bytes in
				con.byte[1] = memspace[pc + 2];			// little endian order
				con.byte[2] = memspace[pc + 3];			//
				con.byte[3] = memspace[pc + 4];			//
				
				value = con.integer;					// This is the offset amount

				if (!(ZF == 1 || (SF ^ OF)))
				{
					pc = value;
				}
				else
				{
					pc += 5;
				}

			break;

			// 80 CALL 32bit destination
			case 0x80:


				con.byte[0] = memspace[pc + 1];			// Getting the value bytes in
				con.byte[1] = memspace[pc + 2];			// little endian order
				con.byte[2] = memspace[pc + 3];			//
				con.byte[3] = memspace[pc + 4];			//
				
				value = con.integer;					// This is the offset amount
				
				reg[4] -= 4;							// %ESP
				
				con.integer = pc + 5;

				memspace[reg[4] + 0] = con.byte[0];	//
				memspace[reg[4] + 1] = con.byte[1];	// Stores the integer at the specified location
				memspace[reg[4] + 2] = con.byte[2];	// in little endian order
				memspace[reg[4] + 3] = con.byte[3];	//

				pc = value;

			break;

			// 90 RET 32bit destination
			case 0x90:
			
				con.byte[0] = memspace[reg[4] + 0];	//
				con.byte[1] = memspace[reg[4] + 1];	// Getting the value bytes
				con.byte[2] = memspace[reg[4] + 2];	// in little endian order
				con.byte[3] = memspace[reg[4] + 3]; //

				pc = con.integer;

				reg[4] += 4;

			break;

			// A0 PUSHL 
			case 0xA0:

				getargs(&arg1, &arg2);

				reg[4] -= 4;

				con.integer = reg[arg1];
				
				memspace[reg[4] + 0] = con.byte[0];	//
				memspace[reg[4] + 1] = con.byte[1];	// Stores the integer at the specified location
				memspace[reg[4] + 2] = con.byte[2];	// in little endian order
				memspace[reg[4] + 3] = con.byte[3];	//

				pc += 2;

			break;

			// B0 POPL
			case 0xB0:

				getargs(&arg1, &arg2);

				con.byte[0] = memspace[reg[4] + 0];			// Getting the value bytes in
				con.byte[1] = memspace[reg[4] + 1];			// little endian order
				con.byte[2] = memspace[reg[4] + 2];			//
				con.byte[3] = memspace[reg[4] + 3];			//

				value = con.integer;

				reg[arg1] = value;
				reg[4] += 4;
				pc += 2;

			break;

			// C0 READB 
			case 0xC0:

				ZF = 0;
				
				getargs(&arg1, &arg2);

				con.byte[0] = memspace[pc + 2];			// Getting the value bytes in
				con.byte[1] = memspace[pc + 3];			// little endian order
				con.byte[2] = memspace[pc + 4];			//
				con.byte[3] = memspace[pc + 5];			//

				value = con.integer;

				if (1 > scanf("%c", &inputchar))
				{
					ZF = 1;
				}
				
				memspace[reg[arg1] + value] = inputchar;

				pc += 6;

			break;

			// C1 READL
			case 0xC1:

				ZF = 0;
				
				getargs(&arg1, &arg2);

				con.byte[0] = memspace[pc + 2];			// Getting the value bytes in
				con.byte[1] = memspace[pc + 3];			// little endian order
				con.byte[2] = memspace[pc + 4];			//
				con.byte[3] = memspace[pc + 5];			//

				// Store the results of the scanf to ensure we exit at the right time
				value = con.integer;
				badscan = scanf("%d", &inputword);
				if (badscan < 1)
				{
					ZF = 1;
				}
				
				con.integer = inputword;
				memspace[reg[arg1]+ value + 0] = con.byte[0];	//
				memspace[reg[arg1]+ value + 1] = con.byte[1];	// Stores the integer at the specified location
				memspace[reg[arg1]+ value + 2] = con.byte[2];	// in little endian order
				memspace[reg[arg1]+ value + 3] = con.byte[3];	//

				pc += 6;

			break;

			// D0 WRTIEB
			case 0xD0:

				getargs(&arg1, &arg2);
				
				con.byte[0] = memspace[pc + 2];			// Getting the value bytes in
				con.byte[1] = memspace[pc + 3];			// little endian order
				con.byte[2] = memspace[pc + 4];			//
				con.byte[3] = memspace[pc + 5];			//

				value = con.integer;
		
				printf("%c", (char)memspace[reg[arg1] + value]);
				pc += 6;

			break;

			// D1 WRITEL
			case 0xD1:

				getargs(&arg1, &arg2);
				
				con.byte[0] = memspace[pc + 2];			// Getting the value bytes in
				con.byte[1] = memspace[pc + 3];			// little endian order
				con.byte[2] = memspace[pc + 4];			//
				con.byte[3] = memspace[pc + 5];			//

				value = con.integer;

				con.byte[0] = memspace[value + reg[arg1] + 0];			// Getting the value bytes in
				con.byte[1] = memspace[value + reg[arg1] + 1];			// little endian order
				con.byte[2] = memspace[value + reg[arg1] + 2];			//
				con.byte[3] = memspace[value + reg[arg1] + 3];			//

				num1 = con.integer;
				printf("%d", num1);
				pc += 6;

			break;

			// E0 MOVSBL
			case 0xE0:

				getargs(&arg1, &arg2);

				con.byte[0] = memspace[pc + 2];			// Getting the value bytes in
				con.byte[1] = memspace[pc + 3];			// little endian order
				con.byte[2] = memspace[pc + 4];			//
				con.byte[3] = memspace[pc + 5];			//

				value = con.integer;
				
				con.integer = reg[arg2];
				inputchar = con.byte[3];
				
				// Get the first bit to see what sign we extend
				if ((inputchar >> 7 & 1) == 0)
				{
					con.byte[0] = inputchar;
					con.byte[1] = 0x00;
					con.byte[2] = 0x00;
					con.byte[3] = 0x00;
				}
				else
				{
					con.byte[0] = inputchar;
					con.byte[1] = 0xff;
					con.byte[2] = 0xff;
					con.byte[3] = 0xff;
				}
					
				con.byte[0] = memspace[reg[arg2]+ value + 0];	//
				con.byte[0] = memspace[reg[arg2]+ value + 1];	// Stores the integer at the specified location
				con.byte[0] = memspace[reg[arg2]+ value + 2];	// in little endian order
				con.byte[0] = memspace[reg[arg2]+ value + 3];	//

				reg[arg1] = con.integer;
				pc += 6;

			break;
			
			// Invalid instruction encountered			
			default:
				status = INS;
			break;
		}
		value = 0;
		arg1 = arg2 = 0;
	}
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

/*
	Utility function to see how memory is being used
*/

void printmemory (int size)
{
	int i;
	for (i = 0; i < size; i++)
	{
		unsigned char c = memspace[i];
		printf("%x ", c);
	}
	printf("\n");
}

/*
	Utility function to see why the program stopped running
*/

void printstatus ()
{
	printf("Execution halted:\t");
	switch (status)
	{
		case AOK:
			printf("AOK, program execution successful!\n");
		break;

		case INS:
			printf("INS, invalid instruction encountered during execution.\n");
		break;

		case ADR:
			printf("ADR, invalid address passed to an instruction.\n");
		break;

		case HLT:
			printf("HLT, halt instruction encountered.\n");
		break;
	}
}

/*
	Gets the high and low portion of a byte encoding of the instructions
	Usually used to determine registers
*/

void getargs(unsigned char * arg1, unsigned char * arg2)
{
	*arg1 = (memspace[pc + 1] & 0xf0) >> 4;
	*arg2 = (memspace[pc + 1] & 0x0f);
}