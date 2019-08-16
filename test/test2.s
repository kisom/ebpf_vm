MOV	r6	5
branch:
MOV	r7	512
BE16	r7
JA	branch
LSH	r7	4
MOD	r7	r6

