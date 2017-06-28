/*
 * easy_string.h
 *
 *  Created on: Feb 19, 2014
 *      Author: nathan
 *
 *  Basic string type. Simulated copy and move semantics. Not as blisteringly
 *  fast as plain char*, but much much safer and easier to work with.
 *  Internally consistent allocation and ownership semantics, plus short
 *  string optimization.
 *
 *  This library defines 2 types:
 *  - Strings are structs with ownership semantics; they always are considered
 *    to own their contents. They can be copied and moved. They should only be
 *    passed by value with care, because this is like a move (the new string
 *    owns its contents) but the old one isn't freed or cleared safely. While
 *    they do not have to be null terminated, most of the API guarantees that
 *    created String objects will have a null terminator.
 *  - StringRefs are non-owning strings. They consist of a const char* and a
 *    size, and can refer to both String objects and C-style char* strings.
 *
 *  Interface notes:
 *  - The methods that use strlen (es_tempn, es_move_cstrn) are declared
 *    static inline, to promote complile-time optimizations of strlen on static
 *    strings.
 *  - Raw copies of strings (String x = y) is basically a move. Once the copy
 *    is finished, you can safely let the old String go out of scope, as the
 *    new string is the owner. Similarly, you can pass strings by value into
 *    give functions ownership; the function becomes responsible for the memory,
 *    and the old string can be let out of scope. You can return strings from
 *    functions for the same reason.
 *  - A String with all 0 bytes is a valid, empty string. This means that you
 *    can calloc() Strings or arrays of Strings.
 *  - In general, we guarentee that String and StringRef objects will always
 *    have valid, non-null pointers to their data, even when the strings are
 *    empty / 0-length.
 */

#pragma once

#include <stdio.h> //For FILE*
#include <string.h> //For strlen

//String type
#define STRING_DATA struct { size_t alloc_size; char* begin; }
typedef struct
{
	size_t size;
	union
	{
		STRING_DATA;
		char shortstr[sizeof(STRING_DATA)];
	};
} String;
#undef STRING_DATA

//StringRef type
typedef struct
{
	size_t size;
	const char* begin;
} StringRef;

//Empty value to initialize to
const static String es_empty_string;

//If you REALLY need a null ref, use es_temp(0)

//Get string pointer from a string
char* es_cstr(String* str);
const char* es_cstrc(const String* str);

//Convenience macros for using in functions that take a char* and size, like memcpy
#define ES_STRINGSIZE(STR) (es_cstr(STR)), ((STR)->size)
#define ES_STRCNSTSIZE(STR) (es_cstrc(STR)), ((STR)->size)
#define ES_SIZESTRING(STR) ((STR)->size), (es_cstr(STR))
#define ES_SIZESTRCNST(STR) ((STR)->size), (es_cstrc(STR))
#define ES_STRINGPRINT(STR) ((int)((STR)->size)), (es_cstrc(STR))
#define ES_STRREFSIZE(REF) ((REF)->begin), ((REF)->size)
#define ES_SIZESTRREF(REF) ((REF)->size), ((REF)->begin)
#define ES_STRREFPRINT(REF) ((int)((REF)->size)), ((REF)->begin)
//For instance: printf("%.*s", ES_STRINGPRINT(&str))

//Free a string without cleaning up. Use at the end of scope.
void es_free(String* str);

//Free and clean up resources.
static inline void es_clear(String* str)
{ es_free(str); *str = es_empty_string; }

//Make new string, by copy
String es_copy(StringRef str);

//Make a new string, transfer ownership
String es_move(String* str);
String es_move_cstrn(char* str, size_t size);
static inline String es_move_cstr(char* str)
{ return es_move_cstrn(str, str ? strlen(str) : 0); }

/*
 * Note that es_move_cstr* will NOT add a null terminator, even if there isn't
 * one present. It's a pure pointer move.
 */

//Create a string from a printf-style format string
String es_printf(const char* format, ...);

//Make a string ref
StringRef es_ref(const String* str);
StringRef es_tempn(const char* str, size_t size);

static inline StringRef es_temp(const char* str)
{ return es_tempn(str, str ? strlen(str) : 0 ); }

//Make a string slice
StringRef es_slice(StringRef ref, size_t offset, size_t size);

//Make a string slice, reusing the buffer
void es_slices(String* str, size_t offset, size_t size);

//Concatenate 2 strings.
String es_cat(StringRef str1, StringRef str2);

//Append to a string.
void es_append(String* str1, StringRef str2);

//Convert a string to lower
String es_tolower(StringRef str);

//Convert a string to an unsigned long, or return an error code
int es_toul(unsigned long* result, StringRef str);
//TODO: other numeric conversions

//Comparison
//compare two strings, just like strcmp
int es_compare(StringRef str1, StringRef str2);

//prefix compare two strings. Return 0 if one string is the prefix of the other.
int es_prefix(StringRef str1, StringRef str2);

//compare string length only. Retuns what strcmp would return if the strings
//are different lengths but otherwise identical.
int es_sizecmp(size_t str1, size_t str2);

// ADVANCED USERS ONLY: helpers for using String as a growing buffer

typedef struct {
	size_t available;
	char* buffer;
} EsBuffer;

EsBuffer es_buffer(String* str);
EsBuffer es_buffer_grow(String* str);
EsBuffer es_buffer_force_grow(String* str, size_t extra);
void es_buffer_commit(String* str, size_t amount);
void es_buffer_commit_unsafe(String* str, size_t amount);
