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
};


#endif /* __LIBEBPF_VM_H */
