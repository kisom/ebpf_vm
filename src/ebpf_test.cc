#include <cassert>
#include <inttypes.h>
#include <iostream>
#include <vector>

#include <ebpf.h>

using namespace std;



static bool
registerTest(struct Machine *m, uint8_t r, uint64_t exp)
{
	uint64_t	v = m->registers[r];

	if (v != exp) {
		fprintf(stderr, "[!] %" PRIx64 " != %" PRIx64 "\n",
			v, exp);
	}
	return m->registers[r] == exp;
}


static void
testInstruction()
{
	struct Instr	instr;
	const uint8_t	compiled[] = {
		0x01, 0x00, 0x00, 0x00, // imm = 1
		0x00, 0x00,		// off = 2
		0x06,			// src=0, dst=r6
		0x07			// opcode = 0x07 (ADDI)
	};
	bool	res = false;

	res = next_instruction(compiled, 8, &instr);
	assert(res);

	assert(instr.imm == 1);
	assert(instr.off == 0);
	assert(instr.src == 0);
	assert(instr.dst == 6);
	assert(instr.op == 7);
}


static void
testProgram1()
{
	struct Machine	m;
	uint8_t         prog[] = {
		0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0xb7, 
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x07, 
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x67, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x2f, 
		0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xdc, 
		0x39, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xc7, 
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x64, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x3c, 
	};
	const size_t    proglen = 0x40;
	bool		res = false;

	res = machine_load(&m, prog, proglen, 512);
	assert(res);

	while (machine_step(&m)) ;
	assert(m.cycles == 8);
	assert(registerTest(&m, 6, 0x14));
}


static void
runAll()
{
	testInstruction();
	testProgram1();
}


int
main(int argc, char *argv[])
{
	runAll();
}
