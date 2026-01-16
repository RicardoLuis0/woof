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

#include "declarate_parse.h"
#include "m_array.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

inline static void ParseErrorVArgs(const char * msg, ...)
{
    va_list args, args2;
    va_start(args, msg);
    va_copy(args2, args);

    size_t len = vsnprintf(NULL, 0, msg, args2);
	
	va_end(args2);

    char * str = malloc(len + 1);

    vsnprintf(str, len + 1, msg, args);
	
	va_end(args);

    str[len] = '\0';

    DoParseError(str, true);
}

inline static void ParseWarningVArgs(const char * msg, ...)
{
    va_list args, args2;
    va_start(args, msg);
    va_copy(args2, args);

    size_t len = vsnprintf(NULL, 0, msg, args2);
	
	va_end(args2);

    char * str = malloc(len + 1);

    vsnprintf(str, len + 1, msg, args);
	
	va_end(args);

    str[len] = '\0';

    DoParseWarning(str, true);
}

inline static void ParseErrorSimple(const char * msg)
{
    DoParseError(msg, false);
}

inline static void ParseWarningSimple(const char * msg)
{
    DoParseWarning(msg, false);
}

//=====================
//
// SCANNER
//
//=====================

const char * token_type_str[TOKEN_TYPE_COUNT] = {
    "Symbol",
    "String",
    "Identifier",
    "Int",
    "Fixed",
    "End of File",
    "Error",
};

// do not rely on C locale
inline static int is_num(char c)
{
    return (c >= '0' && c <= '9');
}

inline static int is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline static int is_alnum(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

inline static int is_ident_start(char c)
{
    return is_alpha(c) || c == '_';
}

inline static int is_ident(char c)
{
    return is_alnum(c) || c == '_';
}

inline static int is_space(char c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

inline static int is_symbol(char c)
{
    return c >= ' ' && c <='~' && !is_alnum(c);
}

inline static char unescape_char(char c)
{
    switch(c)
    {
    case 'a':
        return '\a';
    case 'b':
        return '\b';
    case 'e':
        return 0x1B;
    case 'f':
        return '\f';
    case 'n':
        return '\n';
    case 'r':
        return '\r';
    case 't':
        return '\t';
    case 'v':
        return '\v';
    case '\\':
        return '\\';
    case '"':
        return '\"';
    default:
        ParseWarningVArgs("Unregognized escape character '%c'", c);
        return c;
    }
}

inline static void SkipWhitespace(lex_state_t * ctx)
{
    while(ctx->cur < ctx->length && is_space(ctx->data[ctx->cur]))
    {
        ctx->cur++;
    }
}

inline static token_t ScanString(lex_state_t * ctx)
{
    char * temp_array = NULL;

    size_t start_index = ctx->cur - 1;

    assert(ctx->data[start_index] == '"');

    bool should_unescape = false;

    while(true)
    {
        if(ctx->cur < ctx->length)
        {
            ParseErrorVArgs("Unterminated string at position %zu", start_index);
            return (token_t){.type=TOKEN_ERROR};
        }

        char c = ctx->data[ctx->cur];

        if(c == '\n' || c == '\r')
        {
            ParseErrorVArgs("String crosses newline at position %zu", ctx->cur);
            return (token_t){.type=TOKEN_ERROR};
        }

        if(should_unescape)
        {
            array_push(temp_array, unescape_char(c));
        }
        else if(c == '\\')
        {
            should_unescape = true;
        }
        else if(c == '"')
        {
            //closing "
            ctx->cur++;
            break;
        }
        else
        {
            array_push(temp_array, c);
        }

        ctx->cur++;
    }

    int n = array_size(temp_array);

    char * str = malloc(n + 1);

    if(!str)
    {
        ParseErrorSimple("Failed to allocate string");

        return (token_t){TOKEN_ERROR};
    }

    str[n] = '\0';
    if(n > 0)
    {
        assert(n != INT_MAX);
        memcpy(str, temp_array, n);
    }
    array_free(temp_array);

    return (token_t){.type=TOKEN_STRING,.position=start_index,.str=str};
}

inline static token_t ScanNumber(lex_state_t * ctx)
{
    bool is_fixed = false;

    bool negative = false;

    size_t actual_start_index = ctx->cur;

    if(ctx->data[ctx->cur] == '-' || ctx->data[ctx->cur] == '+')
    {
        negative = (ctx->data[ctx->cur] == '-');
        ctx->cur++;
    }

    int dot_index = 0;
    //TODO support numeric prefixes for radix, ex. 0x..., 0o..., etc

    size_t start_index = ctx->cur;

    while(true)
    {
        if(ctx->cur < ctx->length)
        {
            break;
        }

        char c = ctx->data[ctx->cur];

        if(c == '.' && dot_index > 0)
        {
            is_fixed = true;
        }
        else if(is_symbol(c) || is_space(c))
        {
            break;
        }
        else if(!is_num(c))
        {
            ParseErrorVArgs("Malformed numeric literal at position %zu", ctx->cur);
            return (token_t){.type=TOKEN_ERROR};
        }

        ctx->cur++;
        if(!is_fixed) dot_index++;
    }

    int n = ctx->cur - start_index;

    char * numparsestr = malloc(n + 1); // [Jay] yeah yeah i could just modify the string we're parsing to add a null terminator/etc instead of copying it, but that's kinda shit

    if(!numparsestr)
    {
        ParseErrorSimple("Failed to allocate string");

        return (token_t){TOKEN_ERROR};
    }

    numparsestr[n] = '\0';

    memcpy(numparsestr, ctx->data + start_index, n);

    if(is_fixed)
    {
        //workaround for locale bullshit
        numparsestr[dot_index] = localeconv()->decimal_point[0];

        char* end;
        int fixed = (int)(strtod(numparsestr, &end) * 65536.0);

        free(numparsestr);

        return (token_t){.type=TOKEN_FIXED, .position=actual_start_index, .fixed = negative ? -fixed : fixed};
    }
    else
    {
        char* end;
        int i = strtol(numparsestr, &end, 10);

        free(numparsestr);

        return (token_t){.type=TOKEN_INT, .position=actual_start_index, .i = negative ? -i : i};
    }
    return (token_t){TOKEN_ERROR};
}

inline static token_t ScanIdentifier(lex_state_t * ctx)
{
    size_t start_index = ctx->cur;

    char * temp_array = NULL;

    char c = ctx->data[ctx->cur];

    assert(is_ident_start(c));

    array_push(temp_array, c);

    for(;(ctx->cur < ctx->length) && ((c = ctx->data[ctx->cur]), is_ident(c)); ctx->cur++)
    {
        array_push(temp_array, c);
    }

    array_push(temp_array, '\0');

    name_t index = LookupNameIndex(temp_array);

    array_free(temp_array);

    return (token_t){.type=TOKEN_IDENTIFIER,.position=start_index,.identifier=index};
}

inline static token_t ScanToken(lex_state_t * ctx)
{
    SkipWhitespace(ctx);

    if(ctx->cur < ctx->length) return (token_t){.type=TOKEN_EOF};

    char c = ctx->data[ctx->cur];

    if(c == '"')
    {
        ctx->cur++;
        return ScanString(ctx);
    }
    else if(is_num(c) || ((c == '-' || c == '+') && ((ctx->cur + 1) < ctx->length) && is_num(ctx->data[ctx->cur + 1]))) // int or fixed
    {
        return ScanNumber(ctx);
    }
    else if(is_ident_start(c)) // [a-zA-Z_] identifier
    {
        return ScanIdentifier(ctx);
    }
    else if(is_symbol(c))
    {
        size_t start_index = ctx->cur;
        ctx->cur++;
        return (token_t){.type=TOKEN_SYMBOL,.position=start_index, .symbol=c};
    }
    else
    {
        ParseErrorVArgs("Unkown character at position %zu", ctx->cur);
        return (token_t){.type=TOKEN_ERROR};
    }
}

token_t NextToken(lex_state_t * ctx)
{
    if(array_size(ctx->buffer) > 0)
    {
        return array_pop(ctx->buffer);
    }
    return ScanToken(ctx);
}

token_t PeekToken(lex_state_t * ctx)
{
    token_t tk = NextToken(ctx);
    UnGetToken(ctx, tk);
    return tk;
}

void UnGetToken(lex_state_t * ctx, token_t tk)
{
    array_push(ctx->buffer, tk);
}

void FreeToken(token_t tk)
{
    if(tk.type == TOKEN_STRING) free(tk.str);
}

bool CheckSymbol(lex_state_t * ctx, char symbol)
{
    token_t tk = NextToken(ctx);

    if(tk.type == TOKEN_SYMBOL && tk.symbol == symbol) return true;

    UnGetToken(ctx, tk);
    return false;
}

bool CheckKeyword(lex_state_t * ctx, name_t identifier)
{
    token_t tk = NextToken(ctx);

    if(tk.type == TOKEN_IDENTIFIER && tk.identifier.index == identifier.index) return true;

    UnGetToken(ctx, tk);
    return false;
}

bool ExpectSymbol(lex_state_t * ctx, char symbol)
{
    token_t tk = NextToken(ctx);

    if(tk.type == TOKEN_ERROR) return false; // invalid token, error was already generated

    if(tk.type != TOKEN_SYMBOL)
    {
        if(tk.type == TOKEN_IDENTIFIER)
        {
            ParseErrorVArgs("Expected '%c' but got %s", symbol, namelist[tk.identifier.index]);
        }
        else
        {
            assert(tk.type >= 0 && tk.type <= TOKEN_TYPE_COUNT);
            const char * tk_name = token_type_str[tk.type];
            ParseErrorVArgs("Expected '%c' but got %s", symbol, tk_name);
        }
        FreeToken(tk);
        return false;
    }
    else if(tk.symbol != symbol)
    {
        ParseErrorVArgs("Expected '%c' but got '%c'", symbol, tk.symbol);
        return false;
    }

    return true;
}

//=====================
//
// PARSER
//
//=====================

#define CONSTANTS_HASHTABLE_SIZE 256

typedef struct
{
    name_t name;
    int value;
} constant_entry_t;

typedef struct
{
    lex_state_t lex;


    name_t symbol_thing; // 'thing'
    name_t symbol_weapon; // 'weapon'
    name_t symbol_ammo; // 'ammo'
    name_t symbol_replaces; // 'replaces'

    constant_entry_t **constants_hashtable;
} parse_state_t;

inline static constant_entry_t * LookupConstant(parse_state_t * ctx, name_t name)
{
    int bucket = name.index % CONSTANTS_HASHTABLE_SIZE;

    constant_entry_t * arr = ctx->constants_hashtable[bucket];

    if(!arr) return NULL;

    int n = array_size(arr);
    for(int i = 0; i < n; i++)
    {
        if(arr[i].name.index == name.index)
        {
            return arr + i;
        }
    }

    return NULL;
}

inline static void SetConstant(parse_state_t * ctx, name_t name, int value)
{
    constant_entry_t * e = LookupConstant(ctx, name);
    if(e)
    {
        e->value = value;
        return;
    }

    int bucket = name.index % CONSTANTS_HASHTABLE_SIZE;

    constant_entry_t * arr = ctx->constants_hashtable[bucket];
    array_push(arr, ((constant_entry_t){.name = name, .value = value}));
    ctx->constants_hashtable[bucket] = arr;
}

inline static void UnexpectedToken(const char * expected, token_t tk)
{
    if(!expected)
    {
        if(tk.type == TOKEN_IDENTIFIER)
        {
            ParseErrorVArgs("Unexpected '%s'", namelist[tk.identifier.index]);
        }
        else if(tk.type == TOKEN_SYMBOL)
        {
            ParseErrorVArgs("Unexpected '%c'", tk.symbol);
        }
        else
        {
            assert(tk.type >= 0 && tk.type <= TOKEN_TYPE_COUNT);
            const char * tk_name = token_type_str[tk.type];
            ParseErrorVArgs("Unexpected %s", tk_name);
        }
    }
    else
    {
        if(tk.type == TOKEN_IDENTIFIER)
        {
            ParseErrorVArgs("Expected %s, got unexpected '%s'", expected, namelist[tk.identifier.index]);
        }
        else if(tk.type == TOKEN_SYMBOL)
        {
            ParseErrorVArgs("Expected %s, got unexpected '%c'", expected, tk.symbol);
        }
        else
        {
            assert(tk.type >= 0 && tk.type <= TOKEN_TYPE_COUNT);
            const char * tk_name = token_type_str[tk.type];
            ParseErrorVArgs("Expected %s, got unexpected %s", expected, tk_name);
        }
    }
    FreeToken(tk);
}

inline static int ParseIndexNameOrConst(parse_state_t * ctx, int * index, name_t * name)
{
    token_t tk = NextToken(&ctx->lex);

    if(tk.type == TOKEN_INT)
    {
        *index = tk.i;
        name->index = 0;
        return true;
    }
    else if(tk.type == TOKEN_IDENTIFIER)
    {
        constant_entry_t * c = LookupConstant(ctx, tk.identifier);

        if(c)
        {
            *index = c->value;
            name->index = 0;
        }
        else
        {
            *index = -1;
            *name = tk.identifier;
        }
        return true;
    }
    else
    {
        UnexpectedToken("identifier or index", tk);
        return false;
    }
}

inline static int ParseValueList(parse_state_t * ctx, declarate_valuelist_t * result)
{
    do
    {
        declarate_valueelem_t t = {0};

        token_t tk = NextToken(&ctx->lex);
        if(tk.type == TOKEN_STRING)
        {
            t.type = DECLARATE_VALUE_STRING;
            t.str = tk.str;
        }
        else if(tk.type == TOKEN_INT)
        {
            t.type = DECLARATE_VALUE_INT;
            t.i = tk.i;
        }
        else if(tk.type == TOKEN_FIXED)
        {
            t.type = DECLARATE_VALUE_FIXED;
            t.fixed = tk.fixed;
        }
        else if(tk.type == TOKEN_IDENTIFIER)
        {
            t.type = DECLARATE_VALUE_FLAGS;
            array_push(t.flags, tk.identifier);
            while(CheckSymbol(&ctx->lex, '|'))
            {
                token_t tk = NextToken(&ctx->lex);
                if(tk.type != TOKEN_IDENTIFIER)
                {
                    UnexpectedToken("identifier", tk);
                    return false;
                }
                array_push(t.flags, tk.identifier);
            }
        }
        else
        {
            UnexpectedToken(NULL, tk);
            return false;
        }
        
        array_push(*result, t);
    }
    while(CheckSymbol(&ctx->lex, ','));
    return true;
}

inline static int IsProperty(parse_state_t * ctx)
{
    token_t tk = NextToken(&ctx->lex);

    if(tk.type == TOKEN_SYMBOL && (tk.symbol == '+' || tk.symbol == '-'))
    { // flag
        UnGetToken(&ctx->lex, tk);
        return true;
    }
    else if(tk.type == TOKEN_IDENTIFIER)
    {
        token_t tk2 = NextToken(&ctx->lex);
        UnGetToken(&ctx->lex, tk2);
        UnGetToken(&ctx->lex, tk);
        return (tk2.type != TOKEN_SYMBOL);
    }
    UnGetToken(&ctx->lex, tk);
    return false;
}

inline static int ParseProperty(parse_state_t * ctx, declarate_propertylist_t * result)
{ // property ::= '+' IDENTIFIER ';' | '-' IDENTIFIER ';' | IDENTIFIER value_list ';'.
    declarate_propertyelem_t p;

    token_t tk = NextToken(&ctx->lex);

    if(tk.type == TOKEN_SYMBOL && (tk.symbol == '+' || tk.symbol == '-'))
    { // flag
        p.flag_remove = (tk.symbol == '-');
        token_t tk = NextToken(&ctx->lex);

        if(tk.type == TOKEN_IDENTIFIER)
        {
            p.name = tk.identifier;
        }
        else
        {
            UnexpectedToken("identifier", tk);
            return false;
        }

        p.params = NULL;

        if(!ExpectSymbol(&ctx->lex, ';')) return false;

        array_push(*result, p);

        return true;
    }
    else if(tk.type == TOKEN_IDENTIFIER)
    { // property
        p.name = tk.identifier;
        p.flag_remove = false;

        if(!ParseValueList(ctx, &p.params)) return false;

        array_push(*result, p);

        return true;
    }
    else
    {
        UnexpectedToken("property or flag", tk);
        return false;
    }
}

inline static int ParseDefinitionShared(parse_state_t * ctx, declarate_def_shared_t * result)
{ // definition_shared ::= IDENTIFIER opt_inherits opt_replaces
    token_t tk = NextToken(&ctx->lex);

    if(tk.type == TOKEN_IDENTIFIER)
    {
        result->name = tk.identifier;
    }
    else
    {
        UnexpectedToken("identifier", tk);
        return false;
    }

    if(CheckSymbol(&ctx->lex, ':'))
    {
        if (!ParseIndexNameOrConst(ctx, &result->inherit, &result->inherit_name)) return false;
    }
    else
    {
        result->inherit = -1;
        result->inherit_name.index = 0;
    }

    if(CheckKeyword(&ctx->lex, ctx->symbol_replaces))
    {
        if (!ParseIndexNameOrConst(ctx, &result->replaces, &result->replaces_name)) return false;
    }
    else
    {
        result->replaces = -1;
        result->replaces_name.index = 0;
    }

    return true;
}

inline static int ParseAmmoDefinition(parse_state_t * ctx, declarate_parse_result_t * result)
{ // ammo_definition ::= 'ammo' IDENTIFIER opt_inherits opt_replaces property_block.
  // property_block  ::= '{' property_list '}'.
    declarate_ammodef_t a;

    ParseDefinitionShared(ctx, (declarate_def_shared_t*) &a);
    
    if(!ExpectSymbol(&ctx->lex, '{')) return false;

    while(IsProperty(ctx))
    {
        if(!ParseProperty(ctx, &a.properties)) return false;
    }

    return ExpectSymbol(&ctx->lex, '}');
}

inline static int ParseWeaponDefinition(parse_state_t * ctx, declarate_parse_result_t * result)
{ // weapon_definition      ::= 'weapon' IDENTIFIER opt_inherits opt_replaces full_definition_block.
  // full_definition_block  ::= '{' property_list states_list '}'.
    declarate_weapondef_t w;

    ParseDefinitionShared(ctx, (declarate_def_shared_t*) &w);

    if(!ExpectSymbol(&ctx->lex, '{')) return false;

    while(IsProperty(ctx))
    {
        if(!ParseProperty(ctx, &w.properties)) return false;
    }

    //TODO parse states

    return ExpectSymbol(&ctx->lex, '}');
}

inline static int ParseThingDefinition(parse_state_t * ctx, declarate_parse_result_t * result)
{ // thing_definition ::= 'thing' IDENTIFIER opt_inherits opt_replaces full_definition_block.
  // full_definition_block  ::= '{' property_list states_list '}'.
    declarate_thingdef_t t;
    
    ParseDefinitionShared(ctx, (declarate_def_shared_t*) &t);

    if(!ExpectSymbol(&ctx->lex, '{')) return false;

    while(IsProperty(ctx))
    {
        if(!ParseProperty(ctx, &t.properties)) return false;
    }

    //TODO parse states

    return ExpectSymbol(&ctx->lex, '}');
}

inline static int ParseDefinition(parse_state_t * ctx, declarate_parse_result_t * result)
{ // definition ::= thing_definition | weapon_definition | ammo_definition.
    token_t tk = NextToken(&ctx->lex);

    if(tk.type == TOKEN_IDENTIFIER)
    {
        if(tk.identifier.index == ctx->symbol_thing.index)
        {
            return ParseThingDefinition(ctx, result);
        }
        else if(tk.identifier.index == ctx->symbol_weapon.index)
        {
            return ParseWeaponDefinition(ctx, result);
        }
        else if(tk.identifier.index == ctx->symbol_ammo.index)
        {
            return ParseAmmoDefinition(ctx, result);
        }
        else
        {
            ParseErrorVArgs("Expected 'thing', 'weapon', or 'ammo', got unexpected '%s'", namelist[tk.identifier.index]);
            return false;
        }
    }

    if(tk.type == TOKEN_EOF)
    {
        return false;
    }
    else
    {
        UnexpectedToken("'thing', 'weapon', or 'ammo'", tk);
        return false;
    }
}

inline static void ParseDeclarate(parse_state_t * ctx, declarate_parse_result_t * result)
{ // declarate ::= { definition }.
    while(ParseDefinition(ctx, result));
}

declarate_parse_result_t parse_declarate(const char * text, size_t length)
{
    declarate_parse_result_t result = {NULL, NULL, NULL};

    parse_state_t state =
    {
        .lex = (lex_state_t){
            .data = text,
            .length = length,
            .cur = 0,
            .buffer = NULL
        },
        .symbol_thing = LookupNameIndex("thing"),
        .symbol_weapon = LookupNameIndex("weapon"),
        .symbol_ammo = LookupNameIndex("ammo"),
        .symbol_replaces = LookupNameIndex("replaces"),
        .constants_hashtable = malloc(sizeof(constant_entry_t *) * CONSTANTS_HASHTABLE_SIZE),
    };

    if(!state.constants_hashtable)
    {
        ParseErrorSimple("Failed to allocate table");

        return result;
    }

    memset(state.constants_hashtable, 0, sizeof(constant_entry_t *) * CONSTANTS_HASHTABLE_SIZE);

    //TODO intialize constants (MT_XXX/MTF_XXX/etc)

    ParseDeclarate(&state, &result);
    
    free(state.constants_hashtable);
    return result;
}

// [Jay] TODO parse declarate