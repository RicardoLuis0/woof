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
//      Declarate parser

#ifndef __DECLARATE_PARSE__
#define __DECLARATE_PARSE__

#include "info.h"
#include "m_array.h"

void DoParseError(const char * msg, int shouldFree); // implementation_defined, shouldFree is a bool, means whether msg is should be free'd or not
void DoParseWarning(const char * msg, int shouldFree); // implementation_defined, shouldFree is a bool, means whether msg is should be free'd or not

typedef enum
{
    TOKEN_SYMBOL,
    TOKEN_STRING,
    TOKEN_IDENTIFIER,
    TOKEN_INT,
    TOKEN_FIXED,
    TOKEN_EOF,
    TOKEN_ERROR,
    TOKEN_TYPE_COUNT
} token_type_e;

extern const char * token_type_str[TOKEN_TYPE_COUNT];

typedef struct
{
    token_type_e type;
    size_t position;
    union
    {
        char symbol;
        char * str;
        int i;
        int fixed;
        name_t identifier;
    };
} token_t;

typedef struct {
    const char * data;
    size_t length;
    size_t cur;

    token_t * buffer;
} lex_state_t;

token_t NextToken(lex_state_t * ctx);

token_t PeekToken(lex_state_t * ctx); // PEEKED TOKENS MUST NOT BE FREE'D, THEY'RE STILL OWNED BY THE PARSER

void UnGetToken(lex_state_t * ctx, token_t tk);


void FreeToken(token_t tk);

bool CheckSymbol(lex_state_t * ctx, char symbol);

bool CheckKeyword(lex_state_t * ctx, name_t identifier);

bool ExpectSymbol(lex_state_t * ctx, char symbol);

typedef enum {
    DECLARATE_VALUE_STRING,
    DECLARATE_VALUE_INT,
    DECLARATE_VALUE_FIXED,
    DECLARATE_VALUE_FLAGS,
} declarate_valuetype_e;

typedef struct
{
    int type;
    union
    {
        char * str;
        int i;
        int fixed;
        name_t * flags; // use array_size(xxx.flags) to iterate
    };
} declarate_valueelem_t;

typedef declarate_valueelem_t * declarate_valuelist_t;

typedef struct
{
    char sprite[4];
    char frame;
    char bright; // bool
    int duration;
    name_t action;
    declarate_valuelist_t params; // use array_size(xxx.params) to iterate
} declarate_statedef_t;

typedef struct
{
    name_t name;
} declarate_labeldef_t;

typedef enum
{
    DECLARATE_GOTO,
    DECLARATE_WAIT,
    DECLARATE_LOOP,
    DECLARATE_STOP,
} declarate_statejump_type_e;

typedef struct
{
    declarate_statejump_type_e type;
    name_t label; // only used for goto, holds name index of state label for jump (should goto support offset-only jumps?)
    int offset; // only used for goto, holds offset for jump
} declarate_statejump_t;

typedef enum
{
    DECLARATE_STATE,
    DECLARATE_LABEL,
    DECLARATE_JUMP,
} declarate_steteelem_type_e;

typedef struct
{
    declarate_steteelem_type_e type;
    union
    {
        declarate_statedef_t state;
        declarate_labeldef_t label;
        declarate_statejump_t jump;
    };
} declarate_stateelem_t;

typedef declarate_stateelem_t * declarate_statelist_t;

typedef struct
{
    name_t name;
    char flag_remove; // boolean, ignored if property isn't a flag, false for '+' flags, true for '-' flags
    declarate_valuelist_t params; // use array_size(xxx.params) to iterate, is null if the property is a flag
} declarate_propertyelem_t;

typedef declarate_propertyelem_t * declarate_propertylist_t;

typedef struct
{
    name_t name;
    int inherit; // -1 = name instead of index (or no inheritance)
    name_t inherit_name; // 0 = no inheritance
    int replaces; // -1 = name instead of index (or no inheritance)
    name_t replaces_name; // 0 = no inheritance
} declarate_def_shared_t;

typedef struct
{
    name_t name;
    int inherit; // -1 = name instead of index (or no inheritance)
    name_t inherit_name; // 0 = no inheritance
    int replaces; // -1 = name instead of index (or no inheritance)
    name_t replaces_name; // 0 = no inheritance
    declarate_propertylist_t properties;
    declarate_statelist_t states;
} declarate_thingdef_t;

typedef struct
{
    name_t name;
    int inherit; // -1 = name instead of index (or no inheritance)
    name_t inherit_name; // 0 = no inheritance
    int replaces; // -1 = name instead of index (or no inheritance)
    name_t replaces_name; // 0 = no inheritance
    declarate_propertylist_t properties;
    declarate_statelist_t states;
} declarate_weapondef_t;

typedef struct
{
    name_t name;
    int inherit; // -1 = name instead of index (or no inheritance)
    name_t inherit_name; // 0 = no inheritance
    int replaces; // -1 = name instead of index (or no inheritance)
    name_t replaces_name; // 0 = no inheritance
    declarate_propertylist_t properties;
} declarate_ammodef_t;

typedef struct
{
    declarate_thingdef_t * things; // use array_size(xxx.things) to iterate
    declarate_weapondef_t * weapons; // use array_size(xxx.weapons) to iterate
    declarate_ammodef_t * ammo; // use array_size(xxx.ammo) to iterate
} declarate_parse_result_t;

declarate_parse_result_t parse_declarate(const char * text, size_t length);

#endif // __DECLARATE_PARSE__