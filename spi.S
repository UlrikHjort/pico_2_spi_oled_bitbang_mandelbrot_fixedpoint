/**************************************************************************
--          PICO2 RISC-V Mandelbrot Fixed Point OLED SPI Bitbang
-- 
--           Copyright (C) 2025 By Ulrik Hørlyk Hjort
--
--  This Program is Free Software; You Can Redistribute It and/or
--  Modify It Under The Terms of The GNU General Public License
--  As Published By The Free Software Foundation; Either Version 2
--  of The License, or (at Your Option) Any Later Version.
--
--  This Program is Distributed in The Hope That It Will Be Useful,
--  But WITHOUT ANY WARRANTY; Without Even The Implied Warranty of
--  MERCHANTABILITY or FITNESS for A PARTICULAR PURPOSE.  See The
--  GNU General Public License for More Details.
--
-- You Should Have Received A Copy of The GNU General Public License
-- Along with This Program; if not, See <Http://Www.Gnu.Org/Licenses/>.
***************************************************************************/
	.section .text
    .global SPI_write_asm_clk_delay
    .type   SPI_write_asm_clk_delay, @function
    .global SPI_write_asm_fast
    .type   SPI_write_asm_fast, @function	

	.equ SIO_BASE,  0xd0000000 
	.equ GPIO_SET,  0x18
	.equ GPIO_CLR, 0x20
	
	.equ MOSI_SHIFTED, 0x08      # 1 << (SPI_MOSI &0x1f) e.g 1 << 3 (MOSI pin is 3 here)
	.equ CLK_SHIFTED, 0x04       # 1 << (SPI_CLK &0x1f)  e,d 1 << 2  (CLK pin is 2 here)

	.extern sleep_ms

##################################################
#
#  No function calls so tmp registers can be used
#  No need to save any registers
#	
# (Delay between clock set/clr not really needed)
##################################################	
SPI_write_asm_fast:
	mv t2, a0 # spi byte cmd/data
	li t5, SIO_BASE
	li t0, 7  # Loop counter, number of bit shifts
	li t3, MOSI_SHIFTED 
	li t4, CLK_SHIFTED  


loop1: # Bit shift loop (7 iterations)
	srl t1, t2, t0   # (byte >> i)
	andi t1,t1,1
	bnez t1, set1
	sw t3, GPIO_CLR(t5) # MOSI low (bit == 0)
	j clr1
set1:
	sw t3, GPIO_SET(t5) # MOSI high (bit == 1)
clr1:
	sw t4, GPIO_SET(t5) # CLK # Clock high
	sw t4, GPIO_CLR(t5) # CLK  # Clock low
	addi t0, t0, -1            # loop counter 
	bge t0, zero, loop1 
	ret



##################################################
#
# Spi write with delay in clock 
#  
#	
# 
##################################################		
SPI_write_asm_clk_delay:
	addi sp, sp, -32     # allocate 32 bytes
	sw ra, 28(sp)        # save return address
	sw s0, 4(sp)
	sw s1, 8(sp)
	sw s2, 12(sp)
	sw s3, 16(sp)
	sw s4, 20(sp)
	sw s5, 24(sp)

	mv s2, a0
	li s5, SIO_BASE
	li s0, 7  # Loop counter, number of bit shifts

	li s3, MOSI_SHIFTED 
	li s4, CLK_SHIFTED  	


loop: # Bit shift loop (7 iterations)
	srl t1, s2, s0   # (byte >> i)
	andi t1,t1,1
	bnez t1, set
	sw s3, GPIO_CLR(s5) # MOSI low (bit == 0)
	j clr
set:
	sw s3, GPIO_SET(s5) # MOSI high (bit == 1)
clr:
	sw s4, GPIO_SET(s5) # CLK # Clock high
	li a0, 1                   # Lo 32-bit part
	li a1,0	                   # Hi 32-bit part	
    jal ra, sleep_us           # Small clock delay (maybe not needed)
	sw s4, GPIO_CLR(s5) # CLK  # Clock low
	li a0, 1                   # Lo 32-bit part
	li a1,0	                   # Hi 32-bit part	
	jal ra, sleep_us	       # Small clock delay (maybe not needed)
	addi s0, s0, -1            # loop counter 
	bge s0, zero, loop 
end:
	lw ra, 28(sp)        # restore return address
	lw s0, 4(sp)
	lw s1, 8(sp)
	lw s2, 12(sp)
	lw s3, 16(sp)
	lw s4, 20(sp)
	lw s5, 24(sp)
	addi sp, sp, 32      # deallocate stack
	ret	



