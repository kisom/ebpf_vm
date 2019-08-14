/*
 * I deeply regret everything about this code except that it works
 * and looks okay.
 */
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <ncurses.h>

#include "ebpf.h"
#include "vm.h"


#define MAX_INSTR_SLEN	38
#define MAX_MESSAGE_LEN	76


struct Machine	vm;
char		messages[MAX_MESSAGE_LEN][11];
uint8_t		message_count = 0;
int		delay = 1;
bool		continuous = false;


static void
finish(int sig)
{
	machine_destroy(&vm);	

	exit(0);
}


static size_t
print_instruction(uint8_t *src, size_t srclen)
{
	struct Instr	 instr;
	char		 outs[MAX_INSTR_SLEN+1];
	char		*name = NULL;

	if (!next_instruction(src, srclen, &instr)) {
		return 0;
	}

	for (int i = 0; i < sizeof(code_table) / sizeof(struct OpCode); i++) {
		if (code_table[i].op == instr.op) {
			name = code_table[i].name;
			break;
		}
	}

	if (name == NULL) {
		return 0;
	}
	assert(name);
	sprintf(outs, "%s %hhx->%hhx @ %hx #%08" PRIx32, name, instr.src,
	        instr.dst, instr.off, instr.imm);
	printf("%s", outs);
	return strnlen(outs, MAX_INSTR_SLEN);
}


static void
print_line(bool endl)
{
	printf("+");
	for (int i = 1; i < 80; i++) printf("-");
	printf("+");
	if (endl) {
		printf("\n");
	}
}


static void
print_padding(char ch, int from, int to)
{
	if (ch == 0) {
		ch = ' ';
	}
	for (int i = from; i < to; i++) {
		printf("%c", ch);
	}
}


static void
print_register(uint8_t reg)
{
	if (reg < 16) {
		printf("| R%" PRId8 ": ", reg);
		if (reg < 10) printf(" ");
		printf("%17" PRIx64 "  ", vm.registers[reg]);
		if (((reg+1) % 3) == 0) printf("  |\n");
	}
	else if (reg == 16) {
		printf("| IP:  %17" PRIx64 "  | FP:  %17" PRIx64 "    |\n",
		       vm.ip, vm.registers[10]);
	}
}


static void
print_registers()
{
	print_line(true);
	for (uint8_t i = 0; i < 17; i++) {
		print_register(i);
	}
	print_line(true);
}


static void
print_messages()
{
	size_t	slen = 0;

	printf("| Messages:");
	print_padding(' ', 11, 80);
	printf("|\n");

	for (uint8_t i = 0; i < 11 ; i++) {
		printf("| ");
		if (i >= message_count) {
			print_padding(' ', 2, 80);
		}
		else {
			slen = strnlen(messages[i], MAX_MESSAGE_LEN);
			printf("%s", messages[i]);
			print_padding(' ', slen+2, 80);
		}
		printf("|\n");
	}
	print_line(false);
}


static void
print_machine()
{
	size_t	 slen = 0;

	printf("\033[2J");
	printf("\033[H");
	print_line(true);
	printf("| EBPF VM");

	for (int i = 9; i < 80; i++) printf(" ");
	printf("|\n");
	print_line(true);
	printf("| INSTR: "); // 8
	
	slen = print_instruction(vm.text + vm.ip, vm.textsz - vm.ip);
	print_padding(' ', slen+8, 52);
	printf("| Cycle: %17" PRIx64, vm.cycles);
	printf(" |\n");

	print_registers();
	print_messages();
	fflush(stdout);
}


static void
shift_messages()
{
	for (uint8_t i = 1; i < 11; i++) {
		memcpy(messages[i-1], messages[i], MAX_MESSAGE_LEN);
	}
	memset(messages[10], 0, MAX_MESSAGE_LEN);
}


static void
add_message(char *msg)
{
	size_t	slen = strnlen(msg, MAX_MESSAGE_LEN);

	if (message_count == 11) {
		shift_messages();
	}
	
	memcpy(messages[message_count], msg, slen);
	if (message_count < 11) {
		message_count++;
	}
}


static void
run_program(char *path)
{
	int	mode = 0;

	if (!machine_load_file(&vm, path, 512)) {
		fprintf(stderr, "[!] failed to load program %s\n", path);
		return;
	}
	
	if (continuous) {
		vm.cont = true;
	}

	add_message("booting...");
	print_machine();
	while (machine_step(&vm)) {
		print_machine();
		if (mode == 0) {
			sleep(delay);
		}
	}
	add_message("execution halted");
	print_machine();
}


static void
usage(FILE *outf, char *argv0, int ex)
{
	fprintf(outf, "Usage: %s [-c] [-t delay] compiled_program\n", argv0);
	fprintf(outf, "\tRun the compiled program.\n");
	fprintf(outf, "\nFlags:\n");
	fprintf(outf, "\t-c\t\tRun continuously (reset at the end of the instructions)\n");
	fprintf(outf, "\t-t delay\tSet the delay in seconds between steps\n");
	exit(ex);
}


int
main(int argc, char *argv[])
{
	char	 opt;
	char	*argv0 = argv[0];

	while ((opt = getopt(argc, argv, "cht:")) != -1) {
		switch (opt) {
		case 'c':
			continuous = true;
			break;
		case 'h':
			usage(stdout, argv0, 0);
			break;
		case 't':
			delay = atoi(optarg);
			break;
		default:
			/* not reached */
			fprintf(stderr, "[!] invalid argument %c\n", opt);
			exit(1);
		}
	}

	argv += optind;
	argc -= optind;

	if (argc != 1) {
		fprintf(stderr, "[!] no program provided\n");
		usage(stderr, argv0, 1);
	}

	run_program(argv[0]);

	finish(0);
	return 0;
}
