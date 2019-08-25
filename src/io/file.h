
/**
 *
 * @file file.h
 *
 * Part of the OpenJazz project
 *
 * @par History:
 * - 23rd August 2005: Created OpenJazz.h
 * - 3rd February 2009: Created file.h from parts of OpenJazz.h
 *
 * @par Licence:
 * Copyright (c) 2005-2010 Alister Thomson
 *
 * OpenJazz is distributed under the terms of
 * the GNU General Public License, version 2.0
 *
 */


#ifndef _FILE_H
#define _FILE_H


#include "OpenJazz.h"

#define SDL2

#ifdef SDL2
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#include <stdio.h>


// Classes

/// File i/o
class File {

	private:
		FILE* file;
		char* filePath;

		bool open (const char* path, const char* name, bool write);

	public:
		File                           (const char* name, bool write);
		~File                          ();

		int                getSize     ();
		void               seek        (int offset, bool reset);
		int                tell        ();
		unsigned char      loadChar    ();
		void               storeChar   (unsigned char val);
		unsigned short int loadShort   ();
		unsigned short int loadShort   (unsigned short int max);
		void               storeShort  (unsigned short int val);
		signed int         loadInt     ();
		void               storeInt    (signed int val);
		unsigned char*     loadBlock   (int length);
		unsigned char*     loadRLE     (int length);
		void               skipRLE     ();
		unsigned char*     loadLZ      (int compressedLength, int length);
		char*              loadString  ();
		SDL_Surface*       loadSurface (int width, int height);
		unsigned char*     loadPixels  (int length);
		unsigned char*     loadPixels  (int length, int key);
		void               loadPalette (SDL_Color* palette, bool rle = true);

};

/// Directory path
class Path {

	public:
		Path* next; ///< Next path to check
		char* path; ///< Path

		Path  (Path* newNext, char* newPath);
		~Path ();

};


// Variable

EXTERN Path* firstPath; ///< Paths to files

#endif

