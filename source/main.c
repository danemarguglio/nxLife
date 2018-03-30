#ifndef SWITCH_H
#define SWITCH_H
#include <switch.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>




// Holds information about the screen and drawing grid
struct canvas {
	u32* fb;		// Pointer to frame buffer (makes drawing easier)
	u32* cm;		// Conversion matrix: Calculate this once instead of each draw
					// 	converts between normal x,y to framebuffer coordinates.

	u32 width;		// X,Y size of drawing area on screen
	u32 height;		// 

	u32 x_scale;	// How to scale the canvas quads into pixels
	u32 y_scale;	//	scale of 1 = 1 pixel

	u32 x_offset;	// Where on the screen to offset the canvas
	u32 y_offset;	//

	u32 autocenter; // Ensures canvas is in the center of the screen for some scales

	u32 x_quads;	// How many blocks on each axis
	u32 y_quads;	//

	u32* quads; 	// Array of 'quads', which are just square blocks of color to be drawn
};


//Initalize a canvas
void canvas_init(struct canvas* cv, u32* fb, u32 width, u32 height, u32 x_offset, u32 y_offset, u32 x_scale, u32 y_scale, bool autocenter) {
	cv -> fb = fb;
	cv -> width = width;
	cv -> height = height;
	cv -> x_scale = x_scale;
	cv -> y_scale = y_scale;
	cv -> x_quads = width / x_scale;
	cv -> y_quads = height / y_scale;
	cv -> autocenter = autocenter;

	//Make sure the canvas is centered in display
	if (autocenter){
		cv -> x_offset = (width % x_scale)/2;
		cv -> y_offset = (height % y_scale)/2;
	}
	else {
		cv -> x_offset = x_offset;
		cv -> y_offset = y_offset;
	}

	//Set up frame buffer conversion matrix
	cv -> cm = malloc(width * height * sizeof(u32));
	for (u32 i = 0; i < width; ++i){
		for (u32 j = 0; j < height; ++j){
			cv -> cm[j*width + i] = gfxGetFramebufferDisplayOffset(i,j);
		}
	}

	//Make all quads white by default
	cv -> quads = malloc(cv->x_quads * cv->y_quads * sizeof(u32));
	for (u32 i = 0; i < cv->x_quads * cv->y_quads; ++i){
		cv -> quads[i] = 0xFFFFFFFF;
	}

}


// Clean up canvas
void canvas_free(struct canvas* cv){
	free(cv->cm);
	free(cv->quads);
}


// Draw a rectangle at x,y
void draw_quad(u32* fb, u32* cm, u32 width, u32 x, u32 y, u32 w, u32 h, u32 color) {
	for (u32 i = x; i < x + w; ++i){
		for (u32 j = y; j < y + h; ++j){
			fb[cm[j*width + i]] = color;
		}
	}	
}


// Draw all quads in a canvas
void canvas_draw(u32* fb, struct canvas* cv){
	for (u32 x = 0; x < cv->x_quads; ++x){
		for (u32 y = 0; y < cv->y_quads; ++y){
			draw_quad(fb, cv->cm, cv->width,								//framebuffer, conversion matrix, matrix width
				x*cv->x_scale + cv->x_offset, y*cv->y_scale + cv->y_offset,	// x, y
				cv->x_scale, cv->y_scale,									// w, h
				cv->quads[y*cv->x_quads + x]);								// which quad to draw
		}
	}
}


// Easy way to get elements from board that wrap around to the other side of it
u32 get_wrap(u32 *arry, u32 width, u32 height, int x, int y){
	int n_x;
	int n_y;

	if (x < 0)
		n_x = width-1;
	else if (x == width)
		n_x = 0;
	else
		n_x = x;

	if (y < 0)
		n_y = height-1;
	else if (y == height)
		n_y = 0;
	else
		n_y = y;

	return arry[n_y * width + n_x];
}



int main(int argc, char **argv)
{
	gfxInitDefault();
	consoleInit(NULL);
	srand(time(NULL));

	// Frame buffer
	u32* fb = NULL;
	u32 fb_width;
	u32 fb_height;

	// Screen settings
	u32 x_res = 1280;
	u32 y_res = 720;
	u32 x_offset = 0;
	u32 y_offset = 0;
	u32 x_scale = 10;
	u32 y_scale = 10;
	bool autocenter = false;

	struct canvas cv;
	canvas_init(&cv, fb, x_res, y_res, x_offset, y_offset, x_scale, y_scale, autocenter);

	// Game board
	u32 life_width = cv.x_quads;
	u32 life_height = cv.y_quads;

	u32 *life = malloc(life_width * life_height * sizeof(u32));
	u32 *life_n = malloc(life_width * life_height * sizeof(u32));
	
	// Init game board randomly
	for (u32 i = 0; i < life_width * life_height; ++i){
		life[i] = rand()%2;
		life_n[i] = 0;
	}


	// Main loop
	while(appletMainLoop())
	{
		// Scan all the inputs. This should be done once for each frame
		hidScanInput();

		fb = (u32*) gfxGetFramebuffer((u32*)&fb_width, (u32*)&fb_height);

		// Update canvas, white for alive cells, black for dead cells
		for (u32 i = 0; i < life_height*life_width; ++i){
			cv.quads[i] = life[i] ? 0x00000000 : 0xFFFFFFFF;
		}

		// Draw to the screen
		canvas_draw(fb, &cv);

		// Calculate next board
		for (int i = 0; i < life_width; ++i){
			for (int j = 0; j < life_height; ++j){

				int n = 0;

				// top left
				n += get_wrap(life, life_width, life_height, i-1, j+1);
				// left
				n += get_wrap(life, life_width, life_height, i-1, j);
				// bottom left
				n += get_wrap(life, life_width, life_height, i-1, j-1);
				// top
				n += get_wrap(life, life_width, life_height, i, j+1);
				// bottom
				n += get_wrap(life, life_width, life_height, i, j-1);
				// top right
				n += get_wrap(life, life_width, life_height, i+1, j+1);
				// right
				n += get_wrap(life, life_width, life_height, i+1, j);
				// bottom right
				n += get_wrap(life, life_width, life_height, i+1, j-1);

				// if alive
				if (life[j*life_width + i]){
					if (n < 2)
						life_n[j*life_width+i] = 0;
					else if (n > 3)
						life_n[j*life_width+i] = 0;
					else
						life_n[j*life_width+i] = 1;
				}
				else {
					if (n == 3)
						life_n[j*life_width+i] = 1;
					else
						life_n[j*life_width+i] = 0;
				}

			}
		}

		// Update gameboard
		memcpy(life, life_n, life_height * life_width * sizeof(u32));


		// Handle key presses
		u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
		
		if (kDown & KEY_PLUS) 	// Exit on plus key
			break;
		
		if (kDown & KEY_MINUS){	// Reset on minus key
			for (u32 i = 0; i < life_width * life_height; ++i){
				life[i] = rand()%2;
			}
		}

		// Render Frame
		gfxFlushBuffers();
		gfxSwapBuffers();
		gfxWaitForVsync();
	}

	// Clean up
	canvas_free(&cv);
	gfxExit();
	return 0;
}