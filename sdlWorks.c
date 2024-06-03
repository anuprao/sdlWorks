// gcc -o sdl_one sdl_one.c -I /usr/include/SDL -l SDL

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>

//

#include <SDL.h>
#include <SDL_render.h>
#include <SDL_thread.h>

//

#define FB_WIDTH  720
#define FB_HEIGHT 1280

#define EVENTCODE_REFRESH 1

//

SDL_Window* screenWindow;
SDL_Surface* screenSurface;
SDL_Renderer* screenRenderer;

SDL_Surface* fbSurface;
SDL_Renderer* fbRenderer;

SDL_Thread* threadID;
SDL_TimerID timerID;

//

unsigned int cbRefreshTimer(unsigned int interval, void *param)
{
	SDL_Event event;
	SDL_UserEvent userevent;

	userevent.type = SDL_USEREVENT;
	userevent.code = EVENTCODE_REFRESH;
	userevent.data1 = NULL;
	userevent.data2 = NULL;

	event.type = SDL_USEREVENT;
	event.user = userevent;

	SDL_PushEvent(&event);
	return(interval);
}

//

void lockSurface(void)
{
	if(SDL_MUSTLOCK(fbSurface)) 
	{
		SDL_LockSurface(fbSurface);
	}
}

void unlockSurface(void)
{
	if(SDL_MUSTLOCK(fbSurface)) 
	{
		SDL_UnlockSurface(fbSurface);
	}
}

//

typedef struct stBufferAttr {
	int BytesPerPixel;
	unsigned char *pixels;
	int pitch;
} stBufferAttr;

int graphicsMain( void* ptrData )
{
	stBufferAttr* ptrBufferAttr;
	int bytes_per_pixel, pitch;

	//
	
	unsigned char *buffer;
	unsigned char *data;
	unsigned char *pline;

	unsigned char r, g ,b;
	unsigned int w, h, i, j, x, y, start, end; 
	struct timeval tv;

	int posx, posy;

	posx = 0;
	posy = 0;

	printf( "Running thread ...\n");

	//

	ptrBufferAttr = (stBufferAttr*) ptrData ;

	if(NULL == ptrBufferAttr)
	{
		return -1;
	}

	if((NULL == ptrBufferAttr->pixels) || (0 == ptrBufferAttr->BytesPerPixel))
	{
		return -2;
	}

	if(0 == ptrBufferAttr->pitch)
	{
		return -3;
	}

	//

	bytes_per_pixel = ptrBufferAttr->BytesPerPixel;
	buffer = ptrBufferAttr->pixels;
	pitch = ptrBufferAttr->pitch;

	//

	// Blank the buffer contents before draw operations
	memset(buffer, 0x00, FB_HEIGHT*pitch);

	//

	w = FB_WIDTH >> 2;
	h = FB_HEIGHT >> 2;

	gettimeofday(&tv, NULL);
	start = tv.tv_sec * 1000 + tv.tv_usec / 1000;

	do
	{
		r = rand() % 256;
		g = rand() % 256;
		b = rand() % 256;

		x = rand() % (FB_WIDTH - w);
		y = rand() % (FB_HEIGHT - h);

		//printf("width = %d\r\n",width);
		//printf("height = %d\r\n",height);
		//printf("w = %d\r\n",w);
		//printf("h = %d\r\n",h);		
		//printf("x = %d\r\n",x);
		//printf("y = %d\r\n",y);

		data = buffer + (y + posy) * (pitch) + (x + posx) * (bytes_per_pixel);
		//printf("data = %p\r\n",data);

		//
		lockSurface();
		//

		for (i = 0; i < h; i++) 
		{
			pline = data;
			for (j = 0; j < w; j++) 
			{
				*pline = (unsigned char)r;
				pline++;

				*pline = (unsigned char)g;
				pline++;

				*pline = (unsigned char)b;
				pline++;

				*pline = 255;//a;
				pline++;
			}

			data += pitch;
		}

		//
		unlockSurface();
		//

		gettimeofday(&tv, NULL);
		end = tv.tv_sec * 1000 + tv.tv_usec / 1000;

		//
		// sleep(1);		

	} while (end < (start + 5000));

	printf("Done !!!\r\n");	

	//

	return 0;
}

//

void myInit(stBufferAttr* ptrBufferAttr)
{
	int return_code;

	if(NULL == ptrBufferAttr)
	{
		return;
	}

	//

	// Initialize the SDL library
	return_code = SDL_Init(SDL_INIT_VIDEO);

	if(return_code < 0) 
	{
		fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}

	atexit(SDL_Quit);

	// Set the video mode we want

	int width = FB_WIDTH; // 0 means any width
	int height = FB_HEIGHT; // 0 means any height
	int bits_per_pixel = 32; // 0 means use the current bpp
	Uint32 flags = 0; //SDL_RESIZABLE; // make a resizable window

	screenWindow = SDL_CreateWindow("fb_emulator", -1, -1, width, height, flags);
	if(NULL == screenWindow) 
	{
		fprintf(stderr, "Unable to obtain screenWindow: %s\n", SDL_GetError());
		exit(2);
	}

	screenSurface = SDL_GetWindowSurface(screenWindow);
	if(NULL == screenSurface) 
	{
		fprintf(stderr, "Unable to get screen surface: %s\n", SDL_GetError());
		exit(3);
	}

	screenRenderer = SDL_GetRenderer(screenWindow);
	if(NULL == screenRenderer) 
	{
		fprintf(stderr, "Unable to get screen renderer: %s\n", SDL_GetError());
		exit(3);
	}

	//

	// Create a 32-bit surface with the bytes of each pixel in R,G,B,A order, as expected by OpenGL for textures 
	
	Uint32 rmask, gmask, bmask, amask;

#if 0
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#endif

	//

	fbSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, bits_per_pixel, rmask, gmask, bmask, amask);
	if(fbSurface == NULL) 
	{
		fprintf(stderr, "CreateRGBSurface failed: %s\n", SDL_GetError());
		exit(1);
	}

	//

	ptrBufferAttr->BytesPerPixel = fbSurface->format->BytesPerPixel;
	ptrBufferAttr->pixels = fbSurface->pixels;
	ptrBufferAttr->pitch = fbSurface->pitch;	
}

void launchGraphicsThread(stBufferAttr* ptrBufferAttr)
{
	if(NULL == ptrBufferAttr)
	{
		return;
	}

	threadID = SDL_CreateThread( graphicsMain, "graphicsMain", (void*)ptrBufferAttr);
}

void initTimers()
{
	Uint32 delay = (33 / 10) * 10;  /* To round it down to the nearest 10 ms */
	timerID = SDL_AddTimer(delay, cbRefreshTimer, NULL);
}

void eventLoop()
{
	SDL_Event event;
	SDL_bool quit = SDL_FALSE;

	while(SDL_FALSE == quit)
	{
		SDL_WaitEvent(&event);

		if(SDL_QUIT == event.type) 
		{
			quit = SDL_TRUE;
		}
		else if(SDL_USEREVENT == event.type) 
		{
			SDL_UserEvent userevent;
			userevent = event.user;

			if(SDL_USEREVENT == userevent.type)
			{
				if(EVENTCODE_REFRESH == userevent.code)
				{
					//Optional
					//SDL_SetRenderDrawColor(screenRenderer, 0, 0, 0, 255);
					//SDL_RenderClear(screenRenderer);

					//printf( "RefreshEvent ...\n");
					SDL_BlitSurface(fbSurface, NULL, screenSurface, NULL);
					//SDL_UpdateRect(screenSurface, 0, 0, width, height);
				}
			}
		}
		else if(SDL_MOUSEBUTTONUP == event.type) 
		{
			printf( "SDL_MOUSEBUTTONUP ... %d : %d, %d\n", event.button.button, event.button.x, event.button.y);
		}	

		//
		// Additional drawing code

		//
		//SDL_RenderPresent(screenRenderer);

		//Update screen
		SDL_UpdateWindowSurface(screenWindow);
	}
}

void waitForGraphicsThread()
{
	SDL_WaitThread( threadID, NULL );
}

void myDeInit()
{
	
}

int main(int argc, char **argv) 
{
	stBufferAttr tBA;

	// Init Graphics
	myInit(&tBA);

	// Launch Thread
	launchGraphicsThread(&tBA);

	// Init Timer
	initTimers();

	// Event Loop
	eventLoop();

	// Wait for end of Graphics thread
	waitForGraphicsThread();

	// DeInit Graphics
	myDeInit();

	printf("Graphics done !!!\r\n");	

	return 0;
}
