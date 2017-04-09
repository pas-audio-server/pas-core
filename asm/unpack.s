/*	Each chanel is 3 bytes.
	There are two channels per sample 
	This means 6 bytes.

	In two samples there are twelve bytes.
	Twelve bytes are 3 32 bit words.
	0				1				2				
	0	1	2	3	0	1	2	3	0	1	2	3
	L2	L1	L0	R2	R1	R0	L2	L1	L0	R2	R1	R0
	LEFT 0		RIGHT 0		LEFT 1		RIGHT 1

	ldrd is load double word

	more learning:
6400 00ff ff87 6400 00ff ff87 6600 00ff in s24le is
0000 ff64 0000 ff87 0000 ff64 0000 ff87 in s32le

*/

		.text
		.global unpack

unpack:	push	{r4-r7,lr}
top:	ldrd	r4, r5, [r0, #8]
		ldr		r6, [r0, #4]
		// LEFT  0 is r4 shifted right 1 byte
		// RIGHT 0 is ((r4 & 0xff) << 16) | (r5 >> 16)
		// LEFT  1 is ((r5 & 0xffff) << 8) | ((r6 >> 24) & 0xff)
		// RIGHT 1 is (r6 & 0xffffff)
		// BUT what about sign?
		pop		{r4-r7,pc}

		.end
