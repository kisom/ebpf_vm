#ifndef __LIBEBPF_EBPF_H
#define __LIBEBPF_EBPF_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>


struct Instr {
	uint32_t	imm;
	int16_t		off;
	uint8_t		dst : 4;
	uint8_t		src : 4;
	uint8_t		op;
};


#define STATUS_HALT	0x01
#define STATUS_ILLE	0x80
#define STATUS_DBZ	0x40


bool	next_instruction(const uint8_t *src, size_t srclen, struct Instr *instr);


struct Machine {
	// technically, there are 10 registers in eBPF:
	// R0  -> rax
	// R1  -> rdi/argv0
	// R2  -> rsi/argv1
	// R3  -> rdx/argv2
	// R4  -> rcx/argv3
	// R5  -> r8/argv4
	// R6  -> callee saved
	// R7  -> callee saved
	// R8  -> callee saved
	// R9  -> callee saved
	// R10 -> rbp/frame pointer
	uint64_t	 registers[0x10];

	size_t		 ip; // instruction pointer

	uint8_t		*text;
	size_t		 textsz;

	uint8_t		*ram;
	size_t		 ramsz;

	size_t		 cycles;
	bool		 cont;

	uint8_t		 status;
};


void	machine_destroy(struct Machine *m);
bool	machine_init(struct Machine *m, size_t textsz, size_t ramsz);
bool	machine_load(struct Machine *m, uint8_t *src, size_t srclen, size_t ramsz);
bool	machine_load_file(struct Machine *m, const char *path, size_t ramsz);
bool	machine_step(struct Machine *m);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __LIBEBPF_H */
