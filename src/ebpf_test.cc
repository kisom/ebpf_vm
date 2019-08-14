#include <cassert>
#include <iostream>
#include <vector>

extern "C" {
#include <ebpf.h>
}

using namespace std;


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
runAll()
{
	testInstruction();
}


int
main(int argc, char *argv[])
{
	runAll();
}
