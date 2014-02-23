/*
 * good_string.h
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
 *    owns its contents) but the old one isn't cleared safely. String objects
 *    are always null-terminated.
 *  - StringRefs are non-owning strings. They consist of a char* and a size, and
 *    can refer to both String objects and C-style char* strings.
 */

#pragma once

#include <stdio.h>

//String type
#define STRING_DATA struct { char* begin; char* alloc_end; }
typedef struct
{
	union
	{
		STRING_DATA;
		char shortstr[sizeof(STRING_DATA)];
	};
	size_t size;
} String;
#undef STRING_DATA

//StringRef type
typedef struct
{
	const char* begin;
	size_t size;
} StringRef;

//Empty values to initialize to
const static String es_empty_string;
const static StringRef es_null_ref;

//Get string pointer from a string
const char* es_cstr(const String* str);

//Convenience macro for using in functions that take a char* and size
#define ES_STRINGSIZE(STR) (es_cstr(STR)), ((STR)->size)
#define ES_STRREFSIZE(REF) ((REF)->begin), ((REF)->size)

//Free a string without cleaning up. Use at the end of scope.
void es_free(String* str);

//Free and clean up resources.
static inline void es_clear(String* str)
{ es_free(str); *str = es_empty_string; }

//Make new string, by copy
String es_copy(StringRef str);
String es_copy_cstr(const char* str);
String es_copy_cstrn(const char* str, size_t size);

//Make a new string, transfer ownership
String es_move(String* str);
String es_move_cstr(char* str);
String es_move_cstrn(char* str, size_t size);

//Make a string ref
StringRef es_ref(const String* str);
StringRef es_temp(const char* str);
StringRef es_tempn(const char* str, size_t size);

//Make a string slice
StringRef es_slice(StringRef ref, long offset, long size);

//Make a string slice, reusing the buffer
String es_slices(String str, long offset, long size);

// Concatenate 2 strings.
String es_cat(StringRef str1, StringRef str2);

//Append to a string.
String es_append(String str1, StringRef str2);

//Comparison
int es_sizecmp(size_t str1, size_t str2);
int es_compare(StringRef str1, StringRef str2);
int es_prefix(StringRef str1, StringRef str2);

// Read up to a delimiting character from the FILE*. Adds null terminator
String es_readline(FILE* stream, char delim, size_t max);
//Note that the returned string may have max+1 characters, if max is reached,
//for the null terminator.

String es_readanyline(FILE* stream, char delim);
