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

#include "doomtype.h"
#include "m_array.h"
#include "p_map.h"
#include "declarate_extra.h"
#include "declarate_parse.h"

#include <stdio.h>

char ** namelist = NULL;
int * nametypelist = NULL;
mobjinfo_t * namedmobjs = NULL;
named_mobjinfo_data_t * namedmobjdata = NULL;

void DoParseError(const char * msg, int shouldFree)
{
    I_Error("%s", msg);
}

void DoParseWarning(const char * msg, int shouldFree)
{
    printf("%s", msg); // TODO how to handle this better for woof?

    if(shouldFree) free((void*)msg);
}


void declarate_InitNamedMobjInfo(void)
{
    array_push(namedmobjs, ((mobjinfo_t){0})); // thing 0 = null thing
    array_push(namedmobjdata, ((named_mobjinfo_data_t){0}));

    array_push(namelist, NULL); // name 0 = null
    array_push(nametypelist, 0); // null string = null thing
}

name_t LookupNameIndex(const char * name)
{
    int n = array_size(namelist);
    for(int i = 1; i < n; i++)
    { // TODO change this to a hashmap if it turns out to be slow enough to matter
        if(strcasecmp(name, namelist[i]) == 0)
        {
            return (name_t){i};
        }
    }
    //no name found, allocate new one
    char * nam = strdup(name);
    array_push(namelist, nam);
    array_push(nametypelist, 0); // don't assign a thing just yet, but reserve the spot
    return (name_t){n};
}

namedtype_t LookupTypeIndex(name_t name)
{
    if(name.index <= 0 || name.index >= array_size(nametypelist))
    {
        return (namedtype_t){TYPE_NULL};
    }
    return (namedtype_t){nametypelist[name.index]};
}

int declarate_NewNamedMobj(name_t name, int copyThing, name_t copyName)
{
    if(name.index <= 0 || name.index >= array_size(nametypelist))
    {
        I_Error("invalid name index %d for declarate_NewNamedMobj", name.index);
    }

    if(nametypelist[name.index])
    {
        I_Error("thing named '%s' already exists", namelist[name.index]);
    }

    int n = array_size(namedmobjs);

    nametypelist[name.index] = n;

    array_push(namedmobjs, ((mobjinfo_t){
        .droppeditem = MT_NAMEDTYPE,
        .droppeditem_type = nulltype,
        .infighting_group = IG_DEFAULT,
        .projectile_group = PG_DEFAULT,
        .splash_group = SG_DEFAULT,
        .altspeed = NO_ALTSPEED,
        .meleerange = MELEERANGE
    }));
	
    array_push(namedmobjdata, ((named_mobjinfo_data_t){
        .labels = NULL,
    }));

    if(!(copyThing == MT_NAMEDTYPE && copyName.index == TYPE_NULL))
    {
        if(copyThing == MT_NAMEDTYPE)
        { // copy from named thing
          //TODO [Jay] copy from named thing
        }
        else
        { // copy from original thing
            if(copyThing < 0 || copyThing >= NUMMOBJTYPES)
            {
                I_Error("thing named '%s' trying to inherit from invalid mobj index %d", namelist[name.index], copyThing); // should copying from dehacked-created things be allowed?
            }
            else
            {
                //TODO [Jay] copy from original thing
            }
        }
    }

    return n;
}

void declarate_ReadLump(int lumpNum)
{
    //TODO [Jay] start implementing declarate parsing
}
