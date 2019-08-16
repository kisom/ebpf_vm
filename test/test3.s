MOV	r6,	5
MOV	r7,	0
main:
	SUB	r6,	1
	ADD	r7,	1
	JNE	r6,	0,	main
MOV	r8,	1
