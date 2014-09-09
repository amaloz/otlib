.globl gfmul 
gfmul: 
 #xmm0 holds operand a (128 bits) 
 #xmm1 holds operand b (128 bits) 
 #rdi holds the pointer to output (128 bits) 
 
 movdqa %xmm0, %xmm3 
 pclmulqdq $0, %xmm1, %xmm3 # xmm3 holds a0*b0 
 movdqa %xmm0, %xmm4 
 pclmulqdq $16, %xmm1, %xmm4 #xmm4 holds a0*b1 
 movdqa %xmm0, %xmm5 
 pclmulqdq $1, %xmm1, %xmm5 # xmm5 holds a1*b0 
 movdqa %xmm0, %xmm6 
 pclmulqdq $17, %xmm1, %xmm6 # xmm6 holds a1*b1 
 pxor %xmm5, %xmm4 # xmm4 holds a0*b1 + a1*b0 
 movdqa %xmm4, %xmm5 
 psrldq $8, %xmm4 
 pslldq $8, %xmm5 
 pxor %xmm5, %xmm3 
 pxor %xmm4, %xmm6 # <xmm6:xmm3> holds the result of 
 # the carry-less multiplication of xmm0 by xmm1 
 
 # shift the result by one bit position to the left cope for the fact 
 # that bits are reversed 
 movdqa %xmm3, %xmm7 
 movdqa %xmm6, %xmm8 
 pslld $1, %xmm3 
 pslld $1, %xmm6 
 psrld $31, %xmm7 
 psrld $31, %xmm8 
 movdqa %xmm7, %xmm9 
 pslldq $4, %xmm8 
 pslldq $4, %xmm7 
 psrldq $12, %xmm9 
 por %xmm7, %xmm3 
 por %xmm8, %xmm6 
 por %xmm9, %xmm6 
 
 #first phase of the reduction 
 movdqa %xmm3, %xmm7 
 movdqa %xmm3, %xmm8 
 movdqa %xmm3, %xmm9 
 pslld $31, %xmm7 # packed right shifting << 31 
 pslld $30, %xmm8 # packed right shifting shift << 30 
 pslld $25, %xmm9 # packed right shifting shift << 25 
 pxor %xmm8, %xmm7 # xor the shifted versions 
 pxor %xmm9, %xmm7 
 
 movdqa %xmm7, %xmm8 
 pslldq $12, %xmm7 
 psrldq $4, %xmm8 
 pxor %xmm7, %xmm3 # first phase of the reduction complete 
 movdqa %xmm3,%xmm2 # second phase of the reduction 
 movdqa %xmm3,%xmm4 
 movdqa %xmm3,%xmm5 
 psrld $1, %xmm2 # packed left shifting >> 1 
 psrld $2, %xmm4 # packed left shifting >> 2 
 psrld $7, %xmm5 # packed left shifting >> 7 
 
 pxor %xmm4, %xmm2 # xor the shifted versions 
 pxor %xmm5, %xmm2 
 pxor %xmm8, %xmm2 
 pxor %xmm2, %xmm3 
 pxor %xmm3, %xmm6 # the result is in xmm6 
 movdqu %xmm6, (%rdi) # store the result 
 ret 
