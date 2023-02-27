/* This files provides address values that exist in the system */

#define SDRAM_BASE            0xC0000000
#define FPGA_ONCHIP_BASE      0xC8000000
#define FPGA_CHAR_BASE        0xC9000000

/* Cyclone V FPGA devices */
#define LEDR_BASE             0xFF200000
#define HEX3_HEX0_BASE        0xFF200020
#define HEX5_HEX4_BASE        0xFF200030
#define SW_BASE               0xFF200040
#define KEY_BASE              0xFF200050
#define TIMER_BASE            0xFF202000
#define PIXEL_BUF_CTRL_BASE   0xFF203020
#define CHAR_BUF_CTRL_BASE    0xFF203030

/* VGA colors */
#define WHITE 0xFFFF
#define YELLOW 0xFFE0
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define GREY 0xC618
#define PINK 0xFC18
#define ORANGE 0xFC00

#define ABS(x) (((x) > 0) ? (x) : -(x))

/* Screen size. */
#define RESOLUTION_X 320
#define RESOLUTION_Y 240

/* Constants for animation */
#define BOX_LEN 2
#define NUM_BOXES 8

#define FALSE 0
#define TRUE 1

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

// Begin part1.s for Lab 7

volatile int pixel_buffer_start; // global variable

void swap(int *x, int *y){
    int temp = *x;
    *x = *y;
    *y = temp;
}

void wait_for_vsync(){
    volatile int *pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE; // get address of DMA controller
    register int status;

    *pixel_ctrl_ptr = 1; // start synchronization process by writing 1 to the status register (unusual way ARM does this)

    status = *(pixel_ctrl_ptr + 3); // get the status register in the DMA controller

    // poll until the status register falls back to 0
    while ((status & 0x01) != 0){
        status = *(pixel_ctrl_ptr+3);
    }
}

void plot_pixel(int x, int y, short int line_color)
{
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

void clear_screen(){
    int x = 0, y = 0;
    for (x = 0; x < RESOLUTION_X; x++){
        for (y = 0; y < RESOLUTION_Y; y++){
            plot_pixel(x, y, 0x0000);
        }
    }
}

void draw_line(int x0, int y0, int x1, int y1, short int colour){
    bool is_steep = ABS(y1-y0) > ABS(x1-x0);
    if (is_steep){
        swap(&x0, &y0);
        swap(&x1, &y1);
    }
    if (x0 > x1){
        swap(&x0, &x1);
        swap(&y0, &y1);
    }

    int deltax = x1 - x0;
    int deltay = ABS(y1-y0);
    int error = -(deltax/2);

    int y = y0, x = x0;
    int y_step = 0;
    if (y0 < y1){
        y_step = 1;
    }else{
        y_step = -1;
    }

    for (x = x0; x < x1; x++){
        if (is_steep){
            plot_pixel(y, x, colour);
        }else{
            plot_pixel(x, y, colour);
        }
        error = error + deltay;
        if (error > 0){
            y = y + y_step;
            error = error - deltax;
        }
    }
}

int main(void)
{
    srand(time(NULL));
	volatile int *pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;
	*(pixel_ctrl_ptr + 1) = (int)FPGA_ONCHIP_BASE;
	wait_for_vsync();

	pixel_buffer_start = *pixel_ctrl_ptr;
	clear_screen();

	*(pixel_ctrl_ptr + 1) = (int)SDRAM_BASE;
	pixel_buffer_start = *(pixel_ctrl_ptr + 1);
    clear_screen();

    // do a permanent loop for the animation
    int i = -1, x0 = 40, y = (rand() % 240), x1 = 280;
    while (true){
        // only erase the previous line if it is actually shown on the display
        if (y-2*i >= 0 && y-2*i <= 239){
            //erase the previous line in the buffer
            draw_line(x0, y-2*i, x1, y-2*i, 0x0000);
        }

		// check if we hit boundaries of display
        if ((y <= 0 && i < 0) || (y >= ((int)RESOLUTION_Y-1) && i > 0)){
            i = -(i);
        }

        // draw the new line
        draw_line(x0, y, x1, y, CYAN);

		wait_for_vsync(); // update the display (swap buffers)
        // update the pixel_buffer_start pointer
		pixel_buffer_start = *(pixel_ctrl_ptr + 1);
		y = y+i;
    }
}