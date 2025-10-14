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
#include "pico/stdlib.h"
#include <stdint.h>

#define RST 14
#define DC 15
#define SPI_MOSI 3
#define SPI_CLK 2

#define LOW 0
#define HIGH 1

extern  void SPI_write_asm(uint32_t byte);
extern  void SPI_write_asm_fast(uint32_t byte);

#define SPI_WRITE(byte) SPI_write_asm_fast(byte);

uint8_t framebuffer[128*64/8];


//////////////////////////////////////////////////////
void OLED_reset() {
                gpio_put(RST, LOW);
                sleep_us(1000);  
                gpio_put(RST, HIGH);          
}



//////////////////////////////////////////////////////
void OLED_cmdmode() { gpio_put(DC, LOW);}


//////////////////////////////////////////////////////
void OLED_datamode() { gpio_put(DC, HIGH);}


//////////////////////////////////////////////////////
void OLED_putpixel(uint8_t x, uint8_t y) {
  uint8_t bitmask = 1 << (7 - (y%8));
  size_t pageno = (63-y)/8;
  size_t col = x % 128;

  framebuffer[pageno*128+col] = framebuffer[pageno*128+col] ^ bitmask;
}


//////////////////////////////////////////////////////
void OLED_update() {
  OLED_cmdmode();

  SPI_WRITE(0x20); // Set memory addressing mode
  SPI_WRITE(0x02); // Horizontal addressing mode
  SPI_WRITE(0x40); // Set Display Start Line     
  SPI_WRITE(0xD3); // Set Display Offset     
  SPI_WRITE(0x00);

        for (size_t i = 0; i < 8; i++) {
                OLED_cmdmode();
                SPI_WRITE(0xB0+i);
                SPI_WRITE(0x00); // Lo Col
                SPI_WRITE(0x10); // Hi Col

                for (size_t j =0; j < 128; j++) {
                        OLED_datamode();
                        SPI_WRITE(framebuffer[j+(128*i)]);
                }
        }
}

//////////////////////////////////////////////////////
void OLED_clear() {

        for (size_t i = 0; i < 8; i++) {
                OLED_cmdmode();
                SPI_WRITE(0xB0+i);
                SPI_WRITE(0x00); // Lo Col
                SPI_WRITE(0x10); // Hi Col

                for (size_t j =0; j < 128; j++) {
                        OLED_datamode();
                        SPI_WRITE(0x00);
                }
        }
}



//////////////////////////////////////////////////////
void init_display() {
    OLED_reset();
    OLED_cmdmode();
    SPI_WRITE(0xAE); // Set display OFF                                                                                                                                         
    SPI_WRITE(0xD5); // Set Display Clock Divide Ratio / OSC Frequency                                                                                                          
    SPI_WRITE(0x80); // Display Clock Divide Ratio / OSC Frequency                                                                                                              
    SPI_WRITE(0xA8); // Set Multiplex Ratio                                                                                                                                     
    SPI_WRITE(0x3F); // Multiplex Ratio for 128x64 (64-1)                                                                                                                       
    SPI_WRITE(0xD3); // Set Display Offset                                                                                                                                      
    SPI_WRITE(0x00); // Display Offset                                                                                                                                          
    SPI_WRITE(0x40); // Set Display Start Line                                                                                                                                  
    SPI_WRITE(0x8D); // Set Charge Pump                                                                                                                                         
    SPI_WRITE(0x14); // Charge Pump (0x10 External, 0x14 Internal DC/DC)                                                                                                        
    SPI_WRITE(0x20); // Set memory addressing mode                                                                                                                              
    SPI_WRITE(0x02); // Horizontal addressing mode                                                                                                                              
    SPI_WRITE(0xA1); // Set segment re-map, column address 127 is mapped to SEG0                                                                                                
    SPI_WRITE(0xC8); // Set Com Output Scan Direction                                                                                                                           
    SPI_WRITE(0xDA); // Set COM Hardware Configuration                                                                                                                          
    SPI_WRITE(0x12); // COM Hardware Configuration                                                                                                                              
    SPI_WRITE(0x81); // Set Contrast                                                                                                                                            
    SPI_WRITE(0xCF); // Contrast                                                                                                                                                
    SPI_WRITE(0xD9); // Set Pre-Charge Period                                                                                                                                   
    SPI_WRITE(0xF1); // Set Pre-Charge Period (0x22 External, 0xF1 Internal)                                                                                                    
    SPI_WRITE(0xDB); // Set VCOMH Deselect Level                                                                                                                                
    SPI_WRITE(0x40); // VCOMH Deselect Level                                                                                                                                    
    SPI_WRITE(0xA4); // Set all pixels OFF                                                                                                                                      
    SPI_WRITE(0xA6); // Set display not inverted                                                                                                                                
    SPI_WRITE(0xAF); // Set display On                          
}


#define HEIGHT 64
#define WIDTH 128
#define MAX_ITER 500
#define LIMIT_ITER MAX_ITER-1


// Mandelbrot 8.24 fixed point
#define DECIMAL_PRECISION 24
#define MUL(a,b) (((int64_t) a * (int64_t) b) >> DECIMAL_PRECISION)
#define DIV(a,b) (((int64_t) a << DECIMAL_PRECISION) / (int64_t) b)
#define INT2FIX(i) (i << DECIMAL_PRECISION)

//////////////////////////////////////////////////////
void mandelbrot_int() {  
    const int max_iter = MAX_ITER;

    // xmin = -2.0, xmax = 1.0;
	int32_t xmin = 0xFE000000, xmax = 0x1000000;

    // ymin = -1.5, ymax = 1.5;
	int32_t ymin = 0xFE800000, ymax = 0x1800000;

    for (unsigned int py = 0; py < HEIGHT; py++) {
        for (unsigned int px = 0; px < WIDTH; px++) {

			int32_t x0 = xmin + DIV(MUL((xmax - xmin), INT2FIX(px)),  /*Width = 128 */ 0x80000000);
			int32_t y0 = ymin + DIV(MUL((ymax - ymin), INT2FIX(py)), /*Height = 64 */ 0x40000000);
            int32_t x = 0, y = 0;
            int32_t iter = 0;

			while (MUL(x,x) + MUL(y,y) <= /* 4.0 */ 0x4000000 && iter < max_iter) {			
					int32_t xtemp = MUL(x,x) - MUL(y,y) + x0;					
					y = MUL(MUL(/* 2.0 */ 0x2000000,x),y) + y0;					
                x = xtemp;
                iter++;
            }

            if (iter > LIMIT_ITER) {
              OLED_putpixel(px, py);
            }            
        }
    }
}



//////////////////////////////////////////////////////
void pico_init(void) {
    gpio_init(RST);
    gpio_set_dir(RST, GPIO_OUT);

    gpio_init(DC);
    gpio_set_dir(DC, GPIO_OUT);

    gpio_init(SPI_MOSI);
    gpio_set_dir(SPI_MOSI, GPIO_OUT);

    gpio_init(SPI_CLK);
    gpio_set_dir(SPI_CLK, GPIO_OUT);
	gpio_put(SPI_CLK, LOW);

}


//////////////////////////////////////////////////////
int main() {
		pico_init();		
		init_display();		
		OLED_clear();		
		mandelbrot_int();
		OLED_update();
}
