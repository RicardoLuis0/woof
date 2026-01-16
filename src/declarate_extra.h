//
//  Copyright(C) 2025 Jay
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
// DESCRIPTION:
//      Declarate support

#ifndef __DECLARATE_EXTRA__
#define __DECLARATE_EXTRA__

#include "info.h"

int declarate_NewNamedMobj(name_t nameIndex, int copyThing, name_t copyName);

void declarate_ReadLump(int lumpNum);

void declarate_InitNamedMobjInfo(void);
#endif // __DECLARATE_EXTRA__