#include "include/shell.h"
#include "include/tty.h"
#include "include/stdlib.h"
#include "include/memory_manager.h"

char commands[NCOMMAND][COMMAND_SIZE] = {"help\0", "pd\0", "vd\0"};

void help(int argc, char** argv)
{
	printf("pd - physical dump\n");
	printf("vd - virtual dump\n");
}
void physdump(int argc, char** argv)
{
	physaddr address = 0;
	if (argc != 1) 
	{
		printf("physdump takes one value\n");
		printf("you passed %x value\n", argc);
		return;
	}
	else
	{   
		for (int i = 0; i < argc; i++)
		{
			int j = 0, limit = 0;
			while((argv[i][j] != 0) && (argv[i][j] != 'h')) { j++; }
			j--;
			if (argv[i][1] == 'x') limit = 2;
			else limit = 0;
			for (int k = 0;; j -= 1, k += 4)
			{
				switch(argv[i][j])
				{
					case 'A':
					case 'a':
						address += (10 << k);
						break;
					case 'B':
					case 'b':
						address += (11 << k);
						break;
					case 'C':
					case 'c':
						address += (12 << k);
						break;
					case 'D':
					case 'd':
						address += (13 << k);
						break;
					case 'E':
					case 'e':
						address += (14 << k);
						break;
					case 'F':
					case 'f':
						address += (15 << k);
						break;
					default:
						address += ((argv[i][j] - 48) << k);
				}
				if (j == limit) break;
			}
		}
	}

	unsigned int temp = *((physaddr*)TEMP_PAGE_INFO);
	unsigned char output = 0;
	unsigned short descriptor = 0;
	temp_map_page(address);
	for (int i = 0; i < 16; i++)
	{	
		asm("mov %%ds, %%eax":"=a"(descriptor));
		printf("%x:%x  ", descriptor, address + i * 16);
		for (int j = i * 16; j < ((i + 1) * 16); j++)
		{
			output = ((char*)((size_t)TEMP_PAGE | (address & 0xfff)))[j];
			if (output < 0x10) printf("0");
			printf("%x ", output & 0xff);	
		}
		printf(" ");
		for (int j = i * 16; j < ((i + 1) * 16); j++)
		{
			output = ((char*)((size_t)TEMP_PAGE | (address & 0xfff)))[j];
			if (output < 32) printf(".");
			else printf("%c", output & 0xff);		
		}
		printf("\n");
	}

	temp_map_page(temp);
}
void virtdump(int argc, char** argv)
{
	physaddr address = 0;
	if (argc != 1) 
	{
		printf("virtdump takes one value\n");
		printf("you passed %x value\n", argc);
		return;
	}
	else
	{   
		for (int i = 0; i < argc; i++)
		{
			int j = 0, limit = 0;
			while((argv[i][j] != 0) && (argv[i][j] != 'h')) { j++; }
			j--;
			if (argv[i][1] == 'x') limit = 2;
			else limit = 0;
			for (int k = 0;; j -= 1, k += 4)
			{
				switch(argv[i][j])
				{
					case 'A':
					case 'a':
						address += (10 << k);
						break;
					case 'B':
					case 'b':
						address += (11 << k);
						break;
					case 'C':
					case 'c':
						address += (12 << k);
						break;
					case 'D':
					case 'd':
						address += (13 << k);
						break;
					case 'E':
					case 'e':
						address += (14 << k);
						break;
					case 'F':
					case 'f':
						address += (15 << k);
						break;
					default:
						address += ((argv[i][j] - 48) << k);
				}
				if (j == limit) break;
			}
		}
	}

	unsigned char output = 0;
	unsigned short descriptor = 0;
	for (int i = 0; i < 16; i++)
	{	
		asm("mov %%ds, %%eax":"=a"(descriptor));
		printf("%x:%x  ", descriptor, address + i * 16);
		for (int j = i * 16; j < ((i + 1) * 16); j++)
		{
			output = ((char*)address)[j];
			if (output < 0x10) printf("0");
			printf("%x ", output & 0xff);	
		}
		printf(" ");
		for (int j = i * 16; j < ((i + 1) * 16); j++)
		{
			output = ((char*)address)[j];
			if (output < 32) printf(".");
			else printf("%c", output & 0xff);		
		}
		printf("\n");
	}
}

void do_command(char *command, int argc, char **argv)
{
	int i;
	for (i = 0; i < NCOMMAND; i++)
	{
		if (strcmp(command, commands[i]) == 0) break;
	}
	switch(i)
	{
		case 0:
			help(argc, argv);
			break;
		case 1:
			physdump(argc, argv);
			break;
		case 2:
			virtdump(argc, argv);
			break;
		default:
			printf("Invalid command\n");
			break;
	}

}

void pars_input(char *input, char **command, int *argc, char ***argv)
{
	//Command parsing
	int i = 0, j;
	for (; input[i] == ' '; i++) {}
	for (j = 0; (input[i] != ' ') && (input[i] != 0); i++, j++) {}
	*command = kmalloc(j + 1);
	memcpy(*command, input + i - j, j);
	(*command)[j] = 0;
	if (input[i] == 0) return;

	//Counting argc and total memory size
	int token_count = 0;
	unsigned int total_argv_size = 0;
	for (; input[i] != 0;)
	{	
		for (; input[i] == ' '; i++) {}
		if (input[i] == 0) break;
		for (; (input[i] != ' ') && (input[i] != 0); i++) total_argv_size++;
		token_count++;
	}

	*argc = token_count;
	*argv = kmalloc(*argc * sizeof(void*) + total_argv_size + *argc);
	unsigned int argv_offset = *argc * sizeof(void*);
	for (int c = 0, i = j, j = 0; c < *argc; c++)
	{	
		for (; input[i] == ' '; i++) {}
		if (input[i] == 0) break;
		for (j = i; (input[i] != ' ') && (input[i] != 0); i++){}
		(*argv)[c] = (char*)(((unsigned int)(*argv)) + argv_offset);
		argv_offset += i - j + 1;
		memcpy((*argv)[c], &(input[j]), i - j);
		((*argv)[c])[i - j] = 0;
	}
	kfree(input);
}

void shell()
{
	clear_screen();
	char* input;
	char* command;
	int argc = 0;
	char** argv;
	while(true)
	{
		printf("$ ");
		input = in_string();

		pars_input(input, &command, &argc, &argv);

		do_command(command, argc, argv);

		kfree(command);
		kfree((char*)argv);
	}
}
