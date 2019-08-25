
/**
 *
 * @file video.cpp
 *
 * Part of the OpenJazz project
 *
 * @par History:
 * - 23rd August 2005: Created main.c
 * - 22nd July 2008: Created util.c from parts of main.c
 * - 3rd February 2009: Renamed util.c to util.cpp
 * - 13th July 2009: Created graphics.cpp from parts of util.cpp
 * - 26th July 2009: Renamed graphics.cpp to video.cpp
 *
 * @par Licence:
 * Copyright (c) 2005-2017 Alister Thomson
 *
 * OpenJazz is distributed under the terms of
 * the GNU General Public License, version 2.0
 *
 * @par Description:
 * Contains graphics utility functions.
 *
 */


#include "paletteeffects.h"
#include "video.h"

#ifdef SCALE
	#include <scalebit.h>
#endif

#include "util.h"

#include <string.h>


/**
 * Creates a surface.
 *
 * @param pixels Pixel data to copy into the surface. Can be NULL.
 * @param width Width of the pixel data and of the surface to be created
 * @param height Height of the pixel data and of the surface to be created
 *
 * @return The completed surface
 */
SDL_Surface* createSurface (unsigned char * pixels, int width, int height) {

	SDL_Surface *ret;
	int y;

	// Create the surface
	ret = SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);

	// Set the surface's palette
	//video.restoreSurfacePalette(ret);
	SDL_Color palette[256];
	SDL_SetPaletteColors(ret->format->palette, palette, 0, 256);

	if (pixels) {

		// Upload pixel data to the surface
		if (SDL_MUSTLOCK(ret)) SDL_LockSurface(ret);

		for (y = 0; y < height; y++)
			memcpy(((unsigned char *)(ret->pixels)) + (ret->pitch * y),
				pixels + (width * y), width);

		if (SDL_MUSTLOCK(ret)) SDL_UnlockSurface(ret);

	}

	return ret;

}


/**
 * Create the video output object.
 */
Video::Video () {

	int count;

	screen = NULL;

#ifdef SCALE
	scaleFactor = 1;
#endif

	// Generate the logical palette
	for (count = 0; count < 256; count++)
		logicalPalette[count].r = logicalPalette[count].g =
 			logicalPalette[count].b = count;

	currentPalette = logicalPalette;

	return;

}


/**
 * Find the maximum horizontal and vertical resolutions.
 */
void Video::findMaxResolution () {

#if defined NO_RESIZE || defined SDL2
	maxW = DEFAULT_SCREEN_WIDTH;
	maxH = DEFAULT_SCREEN_HEIGHT;
#else
	SDL_Rect **resolutions;
	int count;

	resolutions = SDL_ListModes(NULL, fullscreen? FULLSCREEN_FLAGS: WINDOWED_FLAGS);

	if (resolutions == (SDL_Rect **)(-1)) {

		maxW = MAX_SCREEN_WIDTH;
		maxH = MAX_SCREEN_HEIGHT;

	} else {

		maxW = SW;
		maxH = SH;

		for (count = 0; resolutions[count] != NULL; count++) {

			if (resolutions[count]->w > maxW) maxW = resolutions[count]->w;
			if (resolutions[count]->h > maxH) maxH = resolutions[count]->h;

		}

		if (maxW > MAX_SCREEN_WIDTH) maxW = MAX_SCREEN_WIDTH;
		if (maxH > MAX_SCREEN_HEIGHT) maxH = MAX_SCREEN_HEIGHT;
	}
#endif

	return;
}


/**
 * Initialise video output.
 *
 * @param width Width of the window or screen
 * @param height Height of the window or screen
 * @param startFullscreen Whether or not to start in full-screen mode
 *
 * @return Success
 */
bool Video::init (int width, int height, bool startFullscreen) {

	fullscreen = startFullscreen;

	if (fullscreen) SDL_ShowCursor(SDL_DISABLE);

	if (!reset(width, height)) {

		logError("Could not set video mode", SDL_GetError());

		return false;

	}

	#ifdef SDL2
	SDL_SetWindowTitle(window, "OpenJazz");
	#else
	SDL_WM_SetCaption("OpenJazz", NULL);
	#endif

	findMaxResolution();

	return true;

}


/**
 * Sets the size of the video window or the resolution of the screen.
 *
 * @param width New width of the window or screen
 * @param height New height of the window or screen
 *
 * @return Success
 */
bool Video::reset (int width, int height) {

	screenW = width;
	screenH = height;

#ifdef SCALE
	if (canvas != screen) SDL_FreeSurface(canvas);
#endif


#ifdef SDL2 // WE HAVE SDL2!!

// The SDL2 window and renderer

// Initalize Color Masks.
Uint32 redMask, greenMask, blueMask;
redMask   = 0x000000ff;
greenMask = 0x0000ff00;
blueMask  = 0x00ff0000;

window = SDL_CreateWindow("", 0, 0, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC); 

// The buffer where the game puts each frame into.
screen = SDL_CreateRGBSurface(SDL_SWSURFACE, DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT, 8, 0, 0, 0, 0);

// The surface into wich we will convert from 8bit paletted to 32bpp RGB
helper_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT, 32, redMask, greenMask, blueMask, 0); 

// THE SDL2 texture
texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
	DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT);

// Sure clear the screen first.. always nice.
SDL_RenderClear(renderer);
SDL_RenderPresent(renderer); 

#else // WE DO NOT HAVE SDL2...

#ifdef NO_RESIZE
	screen = SDL_SetVideoMode(DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT, 8, FULLSCREEN_FLAGS);
#else
	screen = SDL_SetVideoMode(screenW, screenH, 8, fullscreen? FULLSCREEN_FLAGS: WINDOWED_FLAGS);
#endif
	if (!screen) return false;
#endif //SDL2




#ifdef SCALE
	// Check that the scale will fit in the current resolution
	while ( ((screenW/SW < scaleFactor) || (screenH/SH < scaleFactor)) && (scaleFactor > 1) ) {

		scaleFactor--;

	}

	if (scaleFactor > 1) {

		canvasW = screenW / scaleFactor;
		canvasH = screenH / scaleFactor;
		canvas = createSurface(NULL, canvasW, canvasH);

	} else
#endif
    {

		canvasW = screenW;
		canvasH = screenH;
		canvas = screen;

	}

#if !defined(WIZ) && !defined(GP2X)
	expose();
#endif


	/* A real 8-bit display is quite likely if the user has the right video
	card, the right video drivers, the right version of DirectX/whatever, and
	the right version of SDL. In other words, it's not likely enough. If a real
	palette is assumed when
	a) there really is a real palette, there will be an extremely small speed
		gain.
	b) the palette is emulated, there will be a HUGE speed loss.
	Therefore, assume the palette is emulated. */
	/// @todo Find a better way to determine if palette is emulated
	fakePalette = true;

	return true;

}


/**
 * Sets the display palette.
 *
 * @param palette The new palette
 */
void Video::setPalette (SDL_Color *palette) {

	// Make palette changes invisible until the next draw. Hopefully.
	clearScreen(SDL_MapRGB(screen->format, 0, 0, 0));
	flip(0);

	#ifdef SDL2
	/*SDL_Palette *sdlpalette = SDL_AllocPalette(256);
	SDL_SetPaletteColors(sdlpalette, palette, 0, 256);
	SDL_SetSurfacePalette(screen, sdlpalette);*/
	
	SDL_SetPaletteColors(screen->format->palette, palette, 0, 256);

	printf("*****PALETTE IS SET UP AS:**********");

	uint8_t r;
	uint8_t g;
	uint8_t b;
	for (int i = 0; i < 255; i++) {
		r = screen->format->palette->colors[i].r;
		g = screen->format->palette->colors[i].g;
		b = screen->format->palette->colors[i].b;
		printf(" entry %d:  %d %d %d \n", i, r, g, b);
	}
	#else
	SDL_SetPalette(screen, SDL_PHYSPAL, palette, 0, 256);
	#endif

	currentPalette = palette;

	return;

}


/**
 * Returns the current display palette.
 *
 * @return The current display palette
 */
SDL_Color* Video::getPalette () {

	return currentPalette;

}


/**
 * Sets some colours of the display palette.
 *
 * @param palette The palette containing the new colours
 * @param first The index of the first colour in both the display palette and the specified palette
 * @param amount The number of colours
 */
void Video::changePalette (SDL_Color *palette, unsigned char first, unsigned int amount) {

	#ifdef SDL2
	SDL_SetPaletteColors(screen->format->palette, palette, first, amount);
	#else
	SDL_SetPalette(screen, SDL_PHYSPAL, palette, first, amount);
	#endif

	return;

}


/**
 * Restores a surface's palette.
 *
 * @param surface Surface with a modified palette
 */
void Video::restoreSurfacePalette (SDL_Surface* surface) {

	#ifdef SDL2
	SDL_SetPaletteColors(surface->format->palette, logicalPalette, 0, 256);
	#else
	SDL_SetPalette(surface, SDL_LOGPAL, logicalPalette, 0, 256);
	#endif

	return;

}


/**
 * Returns the maximum possible screen width.
 *
 * @return The maximum width
 */
int Video::getMaxWidth () {

	return maxW;

}


/**
 * Returns the maximum possible screen height.
 *
 * @return The maximum height
 */
int Video::getMaxHeight () {

	return maxH;

}


/**
 * Returns the current width of the window or screen.
 *
 * @return The width
 */
int Video::getWidth () {

	return screenW;

}


/**
 * Returns the current height of the window or screen.
 *
 * @return The height
 */
int Video::getHeight () {

	return screenH;

}


#ifdef SCALE
/**
 * Returns the current scaling factor.
 *
 * @return The scaling factor
 */
int Video::getScaleFactor () {

	return scaleFactor;

}


/**
 * Sets the scaling factor.
 *
 * @param newScaleFactor The new scaling factor
 */
int Video::setScaleFactor (int newScaleFactor) {

	if ((SW * newScaleFactor <= screenW) && (SH * newScaleFactor <= screenH)) {

		scaleFactor = newScaleFactor;

		if (screen) reset(screenW, screenH);

	}

	return scaleFactor;

}
#endif

#ifndef FULLSCREEN_ONLY
/**
 * Determines whether or not full-screen mode is being used.
 *
 * @return Whether or not full-screen mode is being used
 */
bool Video::isFullscreen () {

	return fullscreen;

}
#endif


/**
 * Refresh display palette.
 */
void Video::expose () {

	#ifdef SDL2
	SDL_SetPaletteColors(screen->format->palette, logicalPalette, 0, 256);
	SDL_SetPaletteColors(screen->format->palette, currentPalette, 0, 256);
	#else
	SDL_SetPalette(screen, SDL_LOGPAL, logicalPalette, 0, 256);
	SDL_SetPalette(screen, SDL_PHYSPAL, currentPalette, 0, 256);
	#endif

	return;

}


/**
 * Update video based on a system event.
 *
 * @param event The system event. Events not affecting video will be ignored
 */
void Video::update (SDL_Event *event) {

#if !defined(FULLSCREEN_ONLY) || !defined(NO_RESIZE)
	switch (event->type) {

	#ifndef FULLSCREEN_ONLY
		case SDL_KEYDOWN:

			// If Alt + Enter has been pressed, switch between windowed and full-screen mode.
			if ((event->key.keysym.sym == SDLK_RETURN) &&
				(event->key.keysym.mod & KMOD_ALT)) {

				fullscreen = !fullscreen;

				if (fullscreen) SDL_ShowCursor(SDL_DISABLE);

				reset(screenW, screenH);

				if (!fullscreen) SDL_ShowCursor(SDL_ENABLE);

				findMaxResolution();

			}

			break;
    #endif

    #ifndef SDL2
    #ifndef NO_RESIZE 

		case SDL_VIDEORESIZE:

			reset(event->resize.w, event->resize.h);

			break;
    #endif


		case SDL_VIDEOEXPOSE:

			expose();

			break;

    #endif //SDL2
	}
#endif

	return;

}


/**
 * Draw graphics to screen.
 *
 * @param mspf Ticks per frame
 * @param paletteEffects Palette effects to use
 * @param effectsStopped Whether the effects should be applied without advancing
 */
void Video::flip (int mspf, PaletteEffect* paletteEffects, bool effectsStopped) {

	SDL_Color shownPalette[256];

#ifdef SCALE
	if (canvas != screen) {

		// Copy everything that has been drawn so far
		scale(scaleFactor,
			screen->pixels, screen->pitch,
			canvas->pixels, canvas->pitch,
			screen->format->BytesPerPixel, canvas->w, canvas->h);

	}
#endif

	// Apply palette effects
	if (paletteEffects) {

		/* If the palette is being emulated, compile all palette changes and
		apply them all at once.
		If the palette is being used directly, apply all palette effects
		directly. */

		if (fakePalette) {

			memcpy(shownPalette, currentPalette, sizeof(SDL_Color) * 256);

			paletteEffects->apply(shownPalette, false, mspf, effectsStopped);

			#ifdef SDL2
			SDL_SetPaletteColors(screen->format->palette, shownPalette, 0, 256);
			#else
			SDL_SetPalette(screen, SDL_PHYSPAL, shownPalette, 0, 256);
			#endif
		} else {

			paletteEffects->apply(shownPalette, true, mspf, effectsStopped);

		}

	}

	// Show what has been drawn

#ifdef SDL2
	
	uint8_t r,g,b;
	int npixels = screen->w * screen->h;
	uint8_t index;
	for (int i = 0; i < npixels; i++) {
		// Use BGRA, same as the texture format
		index = ((uint8_t*)screen->pixels)[i];
		r = screen->format->palette->colors[index].r;
		g = screen->format->palette->colors[index].g;
		b = screen->format->palette->colors[index].b;
		//printf ("r %d g %d b %d\n", r, g, b);
		uint32_t RGBColor = ((r << 24) | (g << 16) | (b << 8)) | (0xff << 0);
		//memcpy (((uint32_t*)helper_surface->pixels) + i, &RGBColor, 4);
		((uint32_t*)(helper_surface->pixels))[i] = RGBColor;
	}
    SDL_SetPaletteColors(helper_surface->format->palette, canvas->format->palette->colors, 0, 256);
	SDL_BlitSurface(helper_surface, NULL, helper_surface, NULL); 	
    
	SDL_UpdateTexture(texture, NULL, helper_surface->pixels, helper_surface->pitch); 

	// Rendercopy the texture to the renderer, and present on screen!
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer); 
#else
	SDL_Flip(screen);
#endif  //SDL2

	return;

}


/**
 * Fill the screen with a colour.
 *
 * @param index Index of the colour to use
 */
void Video::clearScreen (int index) {

#if defined(CAANOO) || defined(WIZ) || defined(GP2X) || defined(GAMESHELL)
	// always 240 lines cleared to black
	memset(video.screen->pixels, index, 320*240);
#else
	SDL_FillRect(canvas, NULL, index);
#endif

	return;

}


/**
 * Fill a specified rectangle of the screen with a colour.
 *
 * @param x X-coordinate of the left side of the rectangle
 * @param y Y-coordinate of the top of the rectangle
 * @param width Width of the rectangle
 * @param height Height of the rectangle
 * @param index Index of the colour to use
 */
void drawRect (int x, int y, int width, int height, int index) {

	SDL_Rect dst;

	dst.x = x;
	dst.y = y;
	dst.w = width;
	dst.h = height;

	SDL_FillRect(canvas, &dst, index);

	return;

}

