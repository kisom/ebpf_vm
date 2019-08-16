#include <sys/stat.h>
#include <sys/types.h>

#include <assert.h>
#include <endian.h>
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


#define ALU_ADD		0x0
#define ALU_SUB		0x1
#define ALU_MUL		0x2
#define ALU_DIV		0x3
#define ALU_OR		0x4
#define ALU_AND		0x5
#define ALU_LSH		0x6
#define ALU_RSH		0x7
#define ALU_NEG		0x8
#define ALU_MOD		0x9
#define ALU_XOR		0xa
#define ALU_MOV		0xb
#define ALU_ARSH	0xc
#define BYTE_SWAP	0xd

#define CLS_ALU32	0x4
#define CLS_BRANCH	0x5
#define CLS_ALU		0x7

void
machine_alu(struct Machine *m, struct Instr *instr)
{
	uint64_t	tmp;
	uint64_t	rval = instr->imm;

	if (OP_ALU_REG(instr->op)) {
		rval = m->registers[instr->src];
	}

	switch ((instr->op) >> 4) {
	case ALU_ADD:
		m->registers[instr->dst] += rval;
		break;
	case ALU_SUB:
		m->registers[instr->dst] -= rval;
		break;
	case ALU_MUL:
		m->registers[instr->dst] *= rval;
		break;
	case ALU_DIV:
		if (rval == 0) {
			m->status |= STATUS_HALT|STATUS_DBZ;
			return;
		}
		m->registers[instr->dst] /= rval;
		break;
	case ALU_OR:
		m->registers[instr->dst] |= rval;
		break;
	case ALU_AND:
		m->registers[instr->dst] &= rval;
		break;
	case ALU_LSH:
		m->registers[instr->dst] <<= rval;
		break;
	case ALU_RSH:
		m->registers[instr->dst] >>= rval;
		break;
	case ALU_NEG:
		m->registers[instr->dst] = ~(m->registers[instr->dst]);
		break;
	case ALU_MOD:
		if (rval == 0) {
			m->status |= STATUS_HALT|STATUS_DBZ;
			return;
		}
		m->registers[instr->dst] %= rval;
		break;
	case ALU_XOR:
		m->registers[instr->dst] ^= rval;
		break;
	case ALU_MOV:
		m->registers[instr->dst] = rval;
		break;
	case ALU_ARSH:
		/*
		 * tmp stores the sign bit, which we need to restore after the arsh
		 */
		tmp = m->registers[instr->dst] & 0x8000000000000000L;
		m->registers[instr->dst] >>= rval;
		m->registers[instr->dst] += tmp;
		break;
	default:
		m->status |= (STATUS_HALT|STATUS_ILLE);
		abort();
	}
}


void
machine_alu32(struct Machine *m, struct Instr *instr)
{
	uint32_t	lval = (m->registers[instr->dst] & 0xffffffff);
	uint32_t	rval = (instr->imm & 0xffffffff);
	uint32_t	tmp;

	if (OP_ALU_REG(instr->op)) {
		rval = (m->registers[instr->src] & 0xffffffff);
	}

	switch ((instr->op) >> 4) {
	case ALU_ADD:
		lval += rval;
		break;
	case ALU_SUB:
		lval -= rval;
		break;
	case ALU_MUL:
		lval *= rval;
		break;
	case ALU_DIV:
		if (rval == 0) {
			m->status |= STATUS_HALT|STATUS_DBZ;
			return;
		}
		lval /= rval;
		break;
	case ALU_OR:
		lval |= rval;
		break;
	case ALU_AND:
		lval &= rval;
		break;
	case ALU_LSH:
		lval <<= rval;
		break;
	case ALU_RSH:
		lval >>= rval;
		break;
	case ALU_NEG:
		lval = ~(lval);
		break;
	case ALU_MOD:
		if (rval == 0) {
			m->status |= STATUS_HALT|STATUS_DBZ;
			return;
		}
		lval %= rval;
		break;
	case ALU_XOR:
		lval ^= rval;
		break;
	case ALU_MOV:
		lval = rval;
		break;
	case ALU_ARSH:
		/*
		 * tmp stores the sign bit, which we need to restore after the arsh
		 */
		tmp = lval & 0x8000000000000000L;
		lval >>= rval;
		lval += tmp;
		break;
	case BYTE_SWAP:
		/*
		 * the byteswap op codes use the same class as the
		 * ALU32 instructions; in this case, the s bit (bit
		 * 3) is used to indicate whether the conversion is
		 * to little endian (the bit is cleared) or to big
		 * endian (the bit is set).
		 */
		if (OP_ALU_REG(instr->op)) {
			switch (instr->imm) {
			case 16:
				m->registers[instr->dst] =
					htobe16((uint16_t)m->registers[instr->dst]);
				break;
			case 32:
				m->registers[instr->dst] =
					htobe32((uint32_t)m->registers[instr->dst]);
				break;
			case 64:
				m->registers[instr->dst] =
					htobe64(m->registers[instr->dst]);
				break;
			default:
				m->status |= (STATUS_HALT|STATUS_ILLE);
				return;
			}
			return;
		}
		else {
			switch (instr->imm) {
			case 16:
				m->registers[instr->dst] =
					htole16((uint16_t)m->registers[instr->dst]);
				break;
			case 32:
				m->registers[instr->dst] =
					htole32((uint32_t)m->registers[instr->dst]);
				break;
			case 64:
				m->registers[instr->dst] =
					htole64(m->registers[instr->dst]);
				break;
			default:
				m->status |= (STATUS_HALT|STATUS_ILLE);
				return;
			}
			return;
		}
	default:
		m->status |= (STATUS_HALT|STATUS_ILLE);
		return;
	}

	m->registers[instr->dst] = lval;
	m->registers[instr->dst] &= 0xffffffff;
}


#define BRANCH_JA	0x0
#define BRANCH_JEQ	0x01
#define BRANCH_JGT	0x02
#define BRANCH_JGE	0x03
#define BRANCH_JLT	0x0a
#define BRANCH_JLE	0x0b
#define BRANCH_JSET	0x04
#define BRANCH_JNE	0x05


void
machine_branch(struct Machine *m, struct Instr *instr)
{
	uint64_t	lval = m->registers[instr->dst];
	uint64_t	rval = instr->imm;

	if (OP_ALU_REG(instr->op)) {
		rval = m->registers[instr->src];
	}

	switch (instr->op >> 4) {
	case BRANCH_JA:
		break;
	case BRANCH_JEQ:
		if (lval != rval) {
			return;
		}
		break;
	case BRANCH_JGT:
		if (lval <= rval) {
			return;
		}
		break;
	case BRANCH_JGE:
		if (lval < rval) {
			return;
		}
		break;
	case BRANCH_JLT:
		if (lval >= rval) {
			return;
		}
		break;
	case BRANCH_JLE:
		if (lval > rval) {
			return;
		}
		break;
	case BRANCH_JSET:
		if ((lval & rval) == 0) {
			return;
		}
		break;
	case BRANCH_JNE:
		if (lval == rval) {
			return;
		}
		break;
	default:
		m->status |= (STATUS_HALT|STATUS_ILLE);
		return;
	}

	m->ip += instr->off;
}


bool
machine_step(struct Machine *m)
{
	struct Instr	instr;

	if (m->status & STATUS_HALT) {
		return false;
	}

	if (!(next_instruction(m->text+m->ip, m->textsz-m->ip, &instr))) {
		return false;
	}

	switch (instr.op & 0x7) {
	case CLS_ALU:
		machine_alu(m, &instr);
		break;
	case CLS_ALU32:
		machine_alu32(m, &instr);
		break;
	case CLS_BRANCH:
		machine_branch(m, &instr);
		break;
	default:
		m->status |= (STATUS_HALT|STATUS_ILLE);
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
	m->ip = 0;
	m->cont = false;
	m->cycles = 0;
	m->status = 0;

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
