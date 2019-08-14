#ifndef __LIBEBPF_VM_H
#define __LIBEBPF_VM_H


struct OpCode {
	uint8_t	 op;
	char	*name; /* generous! */
};


struct OpCode	code_table[] = {
	{0x07, "ADDI"},
	{0x0f, "ADD"},
	{0x17, "SUBI"},
	{0x1f, "SUB"},
	{0x27, "MULI"},
	{0x2f, "MUL"},
	{0x37, "DIVI"},
	{0x3f, "DIV"},
	{0x47, "ORI"},
	{0x4f, "OR"},
	{0x57, "ANDI"},
	{0x5f, "AND"},
	{0x67, "LSHI"},
	{0x6f, "LSH"},
	{0x77, "RSHI"},
	{0x7f, "RSH"},
	{0x87, "NEG"},
	{0x97, "MODI"},
	{0x9f, "MOD"},
	{0xa7, "XORI"},
	{0xaf, "XOR"},
	{0xb7, "MOVI"},
	{0xbf, "MOV"},
	{0xc7, "ARSHI"},
	{0xcf, "ARSH"},
	{0x04, "ADDI32"},
	{0x0c, "ADD32"},
	{0x14, "SUBI32"},
	{0x1c, "SUB32"},
	{0x24, "MULI32"},
	{0x2c, "MUL32"},
	{0x34, "DIVI32"},
	{0x3c, "DIV32"},
	{0x44, "ORI32"},
	{0x4c, "OR32"},
	{0x54, "ANDI32"},
	{0x5c, "AND32"},
	{0x64, "LSHI32"},
	{0x6c, "LSH32"},
	{0x74, "RSHI32"},
	{0x7c, "RSH32"},
	{0x84, "NEG32"},
	{0x94, "MODI32"},
	{0x9c, "MOD32"},
	{0xa4, "XORI32"},
	{0xac, "XOR32"},
	{0xb4, "MOVI32"},
	{0xbc, "MOV32"},
	{0xc4, "ARSHI32"},
	{0xcc, "ARSH32"},
	{0xd4, "LE"},
	{0xdc, "BE"},
};


#endif /* __LIBEBPF_VM_H */
