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
#define BOX_LEN 3
#define NUM_BOXES 8

#define FALSE 0
#define TRUE 1

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

// Begin part3.c code for Lab 7


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

void draw_box(int x0, int y0, short int colour){
    int x = x0, y = y0;
    for (x = x0; x < x0+(int)BOX_LEN; x++){
        for (y = y0; y < y0+(int)BOX_LEN; y++){
            plot_pixel(x, y, colour);
        }
    }
}

int main(void)
{
    // seed the random function with time to ensure proper randomization
    srand(time(NULL));

    // initial setup for DMA controller and both ON CHIP and SDRAM BUFFERS
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    *(pixel_ctrl_ptr + 1) = 0xC8000000;
    wait_for_vsync();
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen();
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1);
    clear_screen();

    // define locations of coordinates for each box (0-7)
    int positionMatrix[(int)NUM_BOXES][2]; // holds the position of each box in a [0][1] = [x][y] fashion
    int deltaMatrix[(int)NUM_BOXES][2]; // holds the change for each box in a [0][1] = [dx][dy] fashion
    int i = 0, x = 0, y = 0, next_x = 0, next_y = 0, dx = 0, dy = 0, next_dx = 0, next_dy = 0, colour = 0;

    // holds the colours we will use for drawing the boxes
    short int colourArray[8] = {(int)YELLOW, (int)RED, (int)GREEN, (int)BLUE, (int)CYAN, (int)MAGENTA, (int)ORANGE, (int)PINK};

    // initialize all the x and y coordinates in positionMatrix
    // Because boxes are 3x3 our coordinates range from (0,0) to (316, 236) instead of the original (0,0) to (319, 239)
    for (i = 0; i < (int)NUM_BOXES; i++){
        positionMatrix[i][0] = rand() % ((int)RESOLUTION_X - (int)BOX_LEN);
        positionMatrix[i][1] = rand() % ((int)RESOLUTION_Y - (int)BOX_LEN);
    }

    // initalize the change in x and y coordinates in deltaMatrix
    // the values will only be either -1 or 1
    for (i = 0; i < (int)NUM_BOXES; i++){
        deltaMatrix[i][0] = (rand() % 2) * 2 - 1;
        deltaMatrix[i][1] = (rand() % 2) * 2 - 1;
    }

    // continuous animation loop
    while (1)
    {
        //printf("Current buffer is: 0x%08x\n", pixel_buffer_start);
        // erase any boxes and lines that were drawn in the last iteration
        for (i = 0; i < (int)NUM_BOXES; i++){
            // setup temporary variables to make code cleaner
            x = positionMatrix[i][0];
            y = positionMatrix[i][1];
            next_x = positionMatrix[(i+1)%(int)NUM_BOXES][0];
            next_y = positionMatrix[(i+1)%(int)NUM_BOXES][1];
            dx = deltaMatrix[i][0];
            dy = deltaMatrix[i][1];
            next_dx = deltaMatrix[(i+1)%(int)NUM_BOXES][0];
            next_dy = deltaMatrix[(i+1)%(int)NUM_BOXES][1];

            int remove_x = x-2*dx, remove_y = y-2*dy, remove_next_x = next_x-2*next_dx, remove_next_y = next_y-2*next_dy;

            // check if the pixels we are removing are in the frame buffer or not
            if (remove_x < 0){
                remove_x = 1;
            }
            if(remove_x > (int)RESOLUTION_X - (int)BOX_LEN - 1){
                remove_x = (int)RESOLUTION_X - (int)BOX_LEN - 2;
            }
            if (remove_y < 0){
                remove_y = 1;
            }
            if(remove_y > (int)RESOLUTION_Y - (int)BOX_LEN - 1){
                remove_y = (int)RESOLUTION_Y - (int)BOX_LEN - 2;
            }
            if (remove_next_x < 0){
                remove_next_x = 1;
            }
            if(remove_next_x > (int)RESOLUTION_X - (int)BOX_LEN - 1){
                remove_next_x = (int)RESOLUTION_X - (int)BOX_LEN - 2;
            }
            if (remove_next_y < 0){
                remove_next_y = 1;
            }
            if(remove_next_y > (int)RESOLUTION_Y - (int)BOX_LEN - 1){
                remove_next_y = (int)RESOLUTION_Y - (int)BOX_LEN - 2;
            }
            // erase the boxes
            draw_box(remove_x, remove_y, 0x0000);
            // erase the lines
            draw_line(remove_x, remove_y, remove_next_x, remove_next_y, 0x0000);
        }

        // code for drawing the boxes and lines
        for (i = 0; i < (int)NUM_BOXES; i++){
            // setup temporary variables to make code cleaner
            x = positionMatrix[i][0];
            y = positionMatrix[i][1];
            next_x = positionMatrix[(i+1)%(int)NUM_BOXES][0];
            next_y = positionMatrix[(i+1)%(int)NUM_BOXES][1];
            colour = colourArray[i];

            // draw the boxe
            draw_box(x, y, colour);
            // draw the line
            draw_line(x, y, next_x, next_y, colour);
        }

        // code for updating the delta change in the boxes
        for (i = 0; i < (int)NUM_BOXES; i++){
            // setup temporary variables to make code cleaner
            x = positionMatrix[i][0];
            y = positionMatrix[i][1];
            dx = deltaMatrix[i][0];
            dy = deltaMatrix[i][1];

            //printf("Delta for box %d is (%d, %d)\n", i, deltaMatrix[i][0], deltaMatrix[i][1]);
            // check for hitting the edge of the screen in x direction
            if ((x <= 0 && dx < 0) || (x >= ((int)RESOLUTION_X - (int)BOX_LEN - 1) && dx > 0)){
                deltaMatrix[i][0] = -(deltaMatrix[i][0]);
            }

            // check for hitting the edge of the screen in y direction
            if ((y <= 0 && dy < 0) || (y >= ((int)RESOLUTION_Y - (int)BOX_LEN - 1) && dy > 0)){
                deltaMatrix[i][1] = -(deltaMatrix[i][1]);
            }
        }

        // increment the positionMatrix with the deltaMatrix
        for (int i = 0; i < (int)NUM_BOXES; i++){
            positionMatrix[i][0] += deltaMatrix[i][0];
            positionMatrix[i][1] += deltaMatrix[i][1];
        }

        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    }
}