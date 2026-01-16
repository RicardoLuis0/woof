//
// Copyright(C) 2005-2014 Simon Howard
// Copyright(C) 2014 Fabian Greffrath
// Copyright(C) 2021 Roman Fomin
// Copyright(C) 2025 Guilherme Miranda
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//
// Parses [SPRITES] sections in BEX files
//

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "deh_defs.h"
#include "deh_io.h"
#include "deh_main.h"
#include "info.h"
#include "m_array.h"
#include "declarate_extra.h"

//
// DSDHacked Sprites
//

char **sprnames = NULL;
int num_sprites;
static char **deh_spritenames = NULL;
static byte *sprnames_state = NULL;

void DEH_InitSprites(void)
{
    //ensure original sprite names are kept unmodified for declarate

    num_sprites = NUMSPRITES;

    sprnames = NULL;
    array_grow_size(sprnames, NUMSPRITES);
    memcpy(sprnames, original_sprnames, NUMSPRITES * sizeof(*sprnames));

    array_grow_size(deh_spritenames, num_sprites);
    for (int i = 0; i < num_sprites; i++)
    {
        deh_spritenames[i] = strdup(sprnames[i]);
    }

    array_grow_size(sprnames_state, num_sprites);
}

void DEH_FreeSprites(void)
{
    for (int i = 0; i < array_size(deh_spritenames); i++)
    {
        if (deh_spritenames[i])
        {
            free(deh_spritenames[i]);
        }
    }
    array_free(deh_spritenames);
    array_free(sprnames_state);
}

static void SpritesEnsureCapacity(int limit)
{
    if (limit < num_sprites)
    {
        return;
    }

    const int old_num_sprites = num_sprites;

    array_grow_size(sprnames, limit);

    num_sprites = array_size(sprnames);
    const int size_delta = num_sprites - old_num_sprites;

    array_grow_size(sprnames_state, size_delta);
}

int DEH_SpritesGetIndex(const char *key)
{
    for (int i = 0; i < num_sprites; ++i)
    {
        if (sprnames[i]
            && !strncasecmp(sprnames[i], key, 4)
            && !sprnames_state[i])
        {
            sprnames_state[i] = true; // sprite has been edited
            return i;
        }
    }

    return -1;
}

int DEH_SpritesGetOriginalIndex(const char *key)
{
    for (int i = 0; i < array_size(deh_spritenames); ++i)
    {
        if (deh_spritenames[i] && !strncasecmp(deh_spritenames[i], key, 4))
        {
            return i;
        }
    }

    // is it a number?
    for (const char *c = key; *c; c++)
    {
        if (!isdigit(*c))
        {
            return -1;
        }
    }

    int i = atoi(key);
    SpritesEnsureCapacity(i);

    return i;
}

//
// The actual parser
//

static void *DEH_BEXSpritesStart(deh_context_t *context, char *line)
{
    char s[10];

    if (sscanf(line, "%9s", s) == 0 || strcmp("[SPRITES]", s))
    {
        DEH_Warning(context, "Parse error on section start");
    }

    return NULL;
}

static void DEH_BEXSpritesParseLine(deh_context_t *context, char *line, void *tag)
{
    char *spritenum, *value;

    if (!DEH_ParseAssignment(line, &spritenum, &value))
    {
        DEH_Warning(context, "Failed to parse sound assignment");
        return;
    }

    const int len = strlen(value);
    if (len != 4)
    {
        DEH_Warning(context, "Invalid sprite string length");
        return;
    }

    const int match = DEH_SpritesGetOriginalIndex(spritenum);
    if (match >= 0)
    {
        sprnames[match] = strdup(value);
    }
}

deh_section_t deh_section_bex_sprites =
{
    "[SPRITES]",
    NULL,
    DEH_BEXSpritesStart,
    DEH_BEXSpritesParseLine,
    NULL,
    NULL,
};
