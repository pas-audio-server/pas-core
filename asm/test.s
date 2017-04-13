	.arch armv7-a
	.eabi_attribute 28, 1	@ Tag_ABI_VFP_args
	.fpu vfpv3-d16
	.eabi_attribute 20, 1	@ Tag_ABI_FP_denormal
	.eabi_attribute 21, 1	@ Tag_ABI_FP_exceptions
	.eabi_attribute 23, 3	@ Tag_ABI_FP_number_model
	.eabi_attribute 24, 1	@ Tag_ABI_align8_needed
	.eabi_attribute 25, 1	@ Tag_ABI_align8_preserved
	.eabi_attribute 26, 2	@ Tag_ABI_enum_size
	.eabi_attribute 30, 2	@ Tag_ABI_optimization_goals
	.eabi_attribute 34, 1	@ Tag_CPU_unaligned_access
	.eabi_attribute 18, 4	@ Tag_ABI_PCS_wchar_t
	.file	"test.cpp"
@ GNU C++ (Ubuntu/Linaro 5.4.0-6ubuntu1~16.04.4) version 5.4.0 20160609 (arm-linux-gnueabihf)
@	compiled by GNU C version 5.4.0 20160609, GMP version 6.1.0, MPFR version 3.1.4, MPC version 1.0.3
@ GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
@ options passed:  -imultiarch arm-linux-gnueabihf -D_GNU_SOURCE test.cpp
@ -march=armv7-a -mfloat-abi=hard -mfpu=vfpv3-d16 -mthumb -mtls-dialect=gnu
@ -O2 -fverbose-asm -fstack-protector-strong -Wformat -Wformat-security
@ options enabled:  -faggressive-loop-optimizations -falign-functions
@ -falign-jumps -falign-labels -falign-loops -fauto-inc-dec
@ -fbranch-count-reg -fcaller-saves -fchkp-check-incomplete-type
@ -fchkp-check-read -fchkp-check-write -fchkp-instrument-calls
@ -fchkp-narrow-bounds -fchkp-optimize -fchkp-store-bounds
@ -fchkp-use-static-bounds -fchkp-use-static-const-bounds
@ -fchkp-use-wrappers -fcombine-stack-adjustments -fcommon -fcompare-elim
@ -fcprop-registers -fcrossjumping -fcse-follow-jumps -fdefer-pop
@ -fdelete-null-pointer-checks -fdevirtualize -fdevirtualize-speculatively
@ -fdwarf2-cfi-asm -fearly-inlining -feliminate-unused-debug-types
@ -fexceptions -fexpensive-optimizations -fforward-propagate -ffunction-cse
@ -fgcse -fgcse-lm -fgnu-runtime -fgnu-unique -fguess-branch-probability
@ -fhoist-adjacent-loads -fident -fif-conversion -fif-conversion2
@ -findirect-inlining -finline -finline-atomics
@ -finline-functions-called-once -finline-small-functions -fipa-cp
@ -fipa-cp-alignment -fipa-icf -fipa-icf-functions -fipa-icf-variables
@ -fipa-profile -fipa-pure-const -fipa-ra -fipa-reference -fipa-sra
@ -fira-hoist-pressure -fira-share-save-slots -fira-share-spill-slots
@ -fisolate-erroneous-paths-dereference -fivopts -fkeep-static-consts
@ -fleading-underscore -flifetime-dse -flra-remat -flto-odr-type-merging
@ -fmath-errno -fmerge-constants -fmerge-debug-strings
@ -fmove-loop-invariants -fomit-frame-pointer -foptimize-sibling-calls
@ -foptimize-strlen -fpartial-inlining -fpeephole -fpeephole2 -fplt
@ -fprefetch-loop-arrays -freg-struct-return -freorder-blocks
@ -freorder-functions -frerun-cse-after-loop
@ -fsched-critical-path-heuristic -fsched-dep-count-heuristic
@ -fsched-group-heuristic -fsched-interblock -fsched-last-insn-heuristic
@ -fsched-pressure -fsched-rank-heuristic -fsched-spec
@ -fsched-spec-insn-heuristic -fsched-stalled-insns-dep -fschedule-insns
@ -fschedule-insns2 -fsection-anchors -fsemantic-interposition
@ -fshow-column -fshrink-wrap -fsigned-zeros -fsplit-ivs-in-unroller
@ -fsplit-wide-types -fssa-phiopt -fstack-protector-strong -fstdarg-opt
@ -fstrict-aliasing -fstrict-overflow -fstrict-volatile-bitfields
@ -fsync-libcalls -fthread-jumps -ftoplevel-reorder -ftrapping-math
@ -ftree-bit-ccp -ftree-builtin-call-dce -ftree-ccp -ftree-ch
@ -ftree-coalesce-vars -ftree-copy-prop -ftree-copyrename -ftree-cselim
@ -ftree-dce -ftree-dominator-opts -ftree-dse -ftree-forwprop -ftree-fre
@ -ftree-loop-if-convert -ftree-loop-im -ftree-loop-ivcanon
@ -ftree-loop-optimize -ftree-parallelize-loops= -ftree-phiprop -ftree-pre
@ -ftree-pta -ftree-reassoc -ftree-scev-cprop -ftree-sink -ftree-slsr
@ -ftree-sra -ftree-switch-conversion -ftree-tail-merge -ftree-ter
@ -ftree-vrp -funit-at-a-time -fverbose-asm -fzero-initialized-in-bss
@ -masm-syntax-unified -mglibc -mlittle-endian -mpic-data-is-text-relative
@ -msched-prolog -mthumb -munaligned-access -mvectorize-with-neon-quad

	.section	.text._ZSt3hexRSt8ios_base,"axG",%progbits,_ZSt3hexRSt8ios_base,comdat
	.align	2
	.weak	_ZSt3hexRSt8ios_base
	.syntax unified
	.thumb
	.thumb_func
	.type	_ZSt3hexRSt8ios_base, %function
_ZSt3hexRSt8ios_base:
	.fnstart
.LFB726:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r3, [r0, #12]	@ __base_2(D)->_M_flags, __base_2(D)->_M_flags
	bic	r3, r3, #74	@ D.25251, __base_2(D)->_M_flags,
	orr	r3, r3, #8	@ D.25251, D.25251,
	str	r3, [r0, #12]	@ D.25251, MEM[(_Ios_Fmtflags &)__base_2(D) + 12]
	bx	lr	@
	.cantunwind
	.fnend
	.size	_ZSt3hexRSt8ios_base, .-_ZSt3hexRSt8ios_base
	.text
	.align	2
	.global	_Z6DecodePhPi
	.syntax unified
	.thumb
	.thumb_func
	.type	_Z6DecodePhPi, %function
_Z6DecodePhPi:
	.fnstart
.LFB1066:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldrb	r2, [r0, #3]	@ zero_extendqisi2	@ D.25255, MEM[(unsigned char *)buffer_1(D) + 3B]
	ldrb	r3, [r0]	@ zero_extendqisi2	@ *buffer_1(D), *buffer_1(D)
	orr	r3, r3, r2, lsl #8	@ tmp149, *buffer_1(D), D.25255,
	sxth	r3, r3	@ tmp150, tmp149
	str	r3, [r1]	@ tmp150, *samples_11(D)
	ldrb	r2, [r0, #4]	@ zero_extendqisi2	@ D.25255, MEM[(unsigned char *)buffer_1(D) + 4B]
	ldrb	r3, [r0, #5]	@ zero_extendqisi2	@ MEM[(unsigned char *)buffer_1(D) + 5B], MEM[(unsigned char *)buffer_1(D) + 5B]
	orr	r3, r3, r2, lsl #8	@ tmp156, MEM[(unsigned char *)buffer_1(D) + 5B], D.25255,
	sxth	r3, r3	@ tmp157, tmp156
	str	r3, [r1, #4]	@ tmp157, MEM[(int *)samples_11(D) + 4B]
	ldrb	r2, [r0, #9]	@ zero_extendqisi2	@ D.25255, MEM[(unsigned char *)buffer_1(D) + 9B]
	ldrb	r3, [r0, #6]	@ zero_extendqisi2	@ MEM[(unsigned char *)buffer_1(D) + 6B], MEM[(unsigned char *)buffer_1(D) + 6B]
	orr	r3, r3, r2, lsl #8	@ tmp163, MEM[(unsigned char *)buffer_1(D) + 6B], D.25255,
	sxth	r3, r3	@ tmp164, tmp163
	str	r3, [r1, #8]	@ tmp164, MEM[(int *)samples_11(D) + 8B]
	ldrb	r2, [r0, #10]	@ zero_extendqisi2	@ D.25255, MEM[(unsigned char *)buffer_1(D) + 10B]
	ldrb	r3, [r0, #11]	@ zero_extendqisi2	@ MEM[(unsigned char *)buffer_1(D) + 11B], MEM[(unsigned char *)buffer_1(D) + 11B]
	orr	r3, r3, r2, lsl #8	@ tmp170, MEM[(unsigned char *)buffer_1(D) + 11B], D.25255,
	sxth	r3, r3	@ tmp171, tmp170
	str	r3, [r1, #12]	@ tmp171, MEM[(int *)samples_11(D) + 12B]
	bx	lr	@
	.cantunwind
	.fnend
	.size	_Z6DecodePhPi, .-_Z6DecodePhPi
	.section	.text.startup,"ax",%progbits
	.align	2
	.global	main
	.syntax unified
	.thumb
	.thumb_func
	.type	main, %function
main:
	.fnstart
.LFB1067:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}	@
	.save {r4, lr}
	movw	r4, #:lower16:_ZSt4cout	@ tmp131,
	movt	r4, #:upper16:_ZSt4cout	@ tmp131,
	ldr	r3, [r4]	@ cout._vptr.basic_ostream, cout._vptr.basic_ostream
	ldr	r0, [r3, #-12]	@ MEM[(int *)_13 + 4294967284B], MEM[(int *)_13 + 4294967284B]
	add	r0, r0, r4	@, tmp131
	bl	_ZSt3hexRSt8ios_base	@
	mvn	r1, #155	@,
	mov	r0, r4	@, tmp131
	bl	_ZNSolsEi	@
	bl	_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_	@
	ldr	r3, [r4]	@ cout._vptr.basic_ostream, cout._vptr.basic_ostream
	ldr	r0, [r3, #-12]	@ MEM[(int *)_4 + 4294967284B], MEM[(int *)_4 + 4294967284B]
	add	r0, r0, r4	@, tmp131
	bl	_ZSt3hexRSt8ios_base	@
	mvn	r1, #120	@,
	mov	r0, r4	@, tmp131
	bl	_ZNSolsEi	@
	bl	_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_	@
	ldr	r3, [r4]	@ cout._vptr.basic_ostream, cout._vptr.basic_ostream
	ldr	r0, [r3, #-12]	@ MEM[(int *)_7 + 4294967284B], MEM[(int *)_7 + 4294967284B]
	add	r0, r0, r4	@, tmp131
	bl	_ZSt3hexRSt8ios_base	@
	mvn	r1, #155	@,
	mov	r0, r4	@, tmp131
	bl	_ZNSolsEi	@
	bl	_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_	@
	ldr	r3, [r4]	@ cout._vptr.basic_ostream, cout._vptr.basic_ostream
	ldr	r0, [r3, #-12]	@ MEM[(int *)_10 + 4294967284B], MEM[(int *)_10 + 4294967284B]
	add	r0, r0, r4	@, tmp131
	bl	_ZSt3hexRSt8ios_base	@
	mvn	r1, #120	@,
	mov	r0, r4	@, tmp131
	bl	_ZNSolsEi	@
	bl	_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_	@
	movs	r0, #0	@,
	pop	{r4, pc}	@
	.fnend
	.size	main, .-main
	.align	2
	.syntax unified
	.thumb
	.thumb_func
	.type	_GLOBAL__sub_I__Z6DecodePhPi, %function
_GLOBAL__sub_I__Z6DecodePhPi:
	.fnstart
.LFB1075:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}	@
	movw	r4, #:lower16:.LANCHOR0	@ tmp110,
	movt	r4, #:upper16:.LANCHOR0	@ tmp110,
	mov	r0, r4	@, tmp110
	bl	_ZNSt8ios_base4InitC1Ev	@
	mov	r0, r4	@, tmp110
	movw	r2, #:lower16:__dso_handle	@,
	movw	r1, #:lower16:_ZNSt8ios_base4InitD1Ev	@,
	movt	r2, #:upper16:__dso_handle	@,
	movt	r1, #:upper16:_ZNSt8ios_base4InitD1Ev	@,
	pop	{r4, lr}	@
	b	__aeabi_atexit	@
	.cantunwind
	.fnend
	.size	_GLOBAL__sub_I__Z6DecodePhPi, .-_GLOBAL__sub_I__Z6DecodePhPi
	.section	.init_array,"aw"
	.align	2
	.word	_GLOBAL__sub_I__Z6DecodePhPi
	.bss
	.align	2
	.set	.LANCHOR0,. + 0
	.type	_ZStL8__ioinit, %object
	.size	_ZStL8__ioinit, 1
_ZStL8__ioinit:
	.space	1
	.hidden	__dso_handle
	.ident	"GCC: (Ubuntu/Linaro 5.4.0-6ubuntu1~16.04.4) 5.4.0 20160609"
	.section	.note.GNU-stack,"",%progbits
