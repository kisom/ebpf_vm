#include <sys/stat.h>
#include <sys/types.h>

#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ebpf.h"


#define OP_ALU_REG(x) (((x) & (1 << 3)) != 0)


bool
next_instruction(const uint8_t *src, size_t srclen, struct Instr *instr)
{
	uint8_t		tmp;
	size_t		offs = 0;

	if ((srclen % 8) != 0) {
		return false;
	}

	memcpy(&instr->imm, src, sizeof(instr->imm));
	offs += sizeof(instr->imm);

	memcpy(&instr->off, src+offs, sizeof(instr->off));
	offs += sizeof(instr->off);

	tmp = src[offs++];
	instr->src = (tmp & 0xF0) >> 4;
	instr->dst = tmp & 0xF;

	instr->op = src[offs++];

	return true;
}



static void
machine_add(struct Machine *m, struct Instr *instr)
{
	if (OP_ALU_REG(instr->op)) {
		m->registers[instr->dst] += m->registers[instr->src];
	}
	else {
		m->registers[instr->dst] += instr->imm;
	}
}


static void
machine_sub(struct Machine *m, struct Instr *instr)
{
	if (OP_ALU_REG(instr->op)) {
		m->registers[instr->dst] -= m->registers[instr->src];
	}
	else {
		m->registers[instr->dst] -= instr->imm;
	}
}


struct OpHandler {
	uint8_t	opcode;
	void (*handler)(struct Machine *, struct Instr *);
};


struct OpHandler	optable[103] = {
	{0x00, NULL},
	{0x01, NULL},
	{0x02, NULL},
	{0x03, NULL},
	{0x04, NULL},
	{0x05, NULL},
	{0x06, NULL},
	{0x07, machine_add},		/* ADDI */
	{0x08, NULL},
	{0x09, NULL},
	{0x0a, NULL},
	{0x0b, NULL},
	{0x0c, NULL},
	{0x0d, NULL},
	{0x0e, NULL},
	{0x0f, machine_add},		/* ADD */
	{0x10, NULL},
	{0x11, NULL},
	{0x12, NULL},
	{0x13, NULL},
	{0x14, NULL},
	{0x15, NULL},
	{0x16, NULL},
	{0x17, machine_sub},		/* SUBI */
	{0x18, NULL},
	{0x19, NULL},
	{0x1a, NULL},
	{0x1b, NULL},
	{0x1c, NULL},
	{0x1d, NULL},
	{0x1e, NULL},
	{0x1f, machine_sub},		/* SUB */
	{0x20, NULL},
	{0x21, NULL},
	{0x22, NULL},
	{0x23, NULL},
	{0x24, NULL},
	{0x25, NULL},
	{0x26, NULL},
	{0x27, NULL},
	{0x28, NULL},
	{0x29, NULL},
	{0x2a, NULL},
	{0x2b, NULL},
	{0x2c, NULL},
	{0x2d, NULL},
	{0x2e, NULL},
	{0x2f, NULL},
	{0x30, NULL},
	{0x31, NULL},
	{0x32, NULL},
	{0x33, NULL},
	{0x34, NULL},
	{0x35, NULL},
	{0x36, NULL},
	{0x37, NULL},
	{0x38, NULL},
	{0x39, NULL},
	{0x3a, NULL},
	{0x3b, NULL},
	{0x3c, NULL},
	{0x3d, NULL},
	{0x3e, NULL},
	{0x3f, NULL},
	{0x40, NULL},
	{0x41, NULL},
	{0x42, NULL},
	{0x43, NULL},
	{0x44, NULL},
	{0x45, NULL},
	{0x46, NULL},
	{0x47, NULL},
	{0x48, NULL},
	{0x49, NULL},
	{0x4a, NULL},
	{0x4b, NULL},
	{0x4c, NULL},
	{0x4d, NULL},
	{0x4e, NULL},
	{0x4f, NULL},
	{0x50, NULL},
	{0x51, NULL},
	{0x52, NULL},
	{0x53, NULL},
	{0x54, NULL},
	{0x55, NULL},
	{0x56, NULL},
	{0x57, NULL},
	{0x58, NULL},
	{0x59, NULL},
	{0x5a, NULL},
	{0x5b, NULL},
	{0x5c, NULL},
	{0x5d, NULL},
	{0x5e, NULL},
	{0x5f, NULL},
	{0x60, NULL},
	{0x61, NULL},
	{0x62, NULL},
	{0x63, NULL},
	{0x64, NULL},
	{0x65, NULL},
	{0x66, NULL},
};


bool
machine_step(struct Machine *m)
{
	struct Instr	instr;
	bool		valid_instr = false;

	if (!(next_instruction(m->text+m->ip, m->textsz-m->ip, &instr))) {
		return false;
	}

	for (uint8_t i = 0; i < sizeof(optable) / sizeof(struct OpHandler); i++) {
		if (optable[i].opcode == instr.op) {
			optable[i].handler(m, &instr);
			valid_instr = true;
			break;
		}
	}

	if (!valid_instr) {
		// TODO: add status register
		return false;
	}

	m->ip += 8;
	m->cycles++;
	if (m->ip == m->textsz) {
		if (!m->cont) {
			return false;
		}
		m->ip = 0;
	}

	return true;
}


bool
machine_init(struct Machine *m, size_t textsz, size_t ramsz)
{
	if (NULL == (m->text = (uint8_t *)malloc(textsz))) {
		return false;
	}
	m->textsz = textsz;

	if (NULL == (m->ram = (uint8_t *)malloc(ramsz))) {
		free(m->text);
		return false;
	}
	m->ramsz = ramsz;

	memset(m->text, 0, textsz);
	memset(m->ram, 0, ramsz);

	for (int i = 0; i < 16; i++) {
		m->registers[i] = 0;
	}
	m->cont = false;
	m->cycles = 0;

	return true;
}


bool
machine_load(struct Machine *m, uint8_t *src, size_t srclen, size_t ramsz)
{
	if (!machine_init(m, srclen, ramsz)) {
		return false;
	}

	memcpy(m->text, src, srclen);
	return true;
}


bool
machine_load_file(struct Machine *m, const char *path, size_t ramsz)
{
	int	fd;
	ssize_t	flen, rlen;

	fd = open(path, O_RDONLY);
	if (fd == -1) {
		return false;
	}

	flen = lseek(fd, 0, SEEK_END);
	flen = lseek(fd, 0, SEEK_CUR);
	lseek(fd, 0, SEEK_SET);

	if (flen == 0) {
		close(fd);
		return false;
	}

	if (!machine_init(m, (size_t)flen, ramsz)) {
		close(fd);
		return false;
	}

	rlen = read(fd, m->text, m->textsz);
	if (rlen != flen) {
		close(fd);
		return false;
	}

	return true;
}


void
machine_destroy(struct Machine *m)
{
	free(m->text);
	m->textsz = 0;

	free(m->ram);
	m->ramsz = 0;

	m->ip = 0;
}
