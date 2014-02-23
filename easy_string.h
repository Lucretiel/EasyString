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
 */

#pragma once

#include <stdio.h>

/*
 * Strings have ownership semantics. They should only be copied by value with
 * care; use the es_copy and es_move functions instead. Generally copying by
 * value is the same as a move, except that the old string isn't cleared. They
 * are null-terminated
 */
typedef struct
{
	union
	{
		char* begin;
		char shortstr[sizeof(char*)];
	};
	size_t size;
} String;

/*
 * StringRefs do not have ownership. Their lifetime should be bound to that
 * of the String they reference. They can by copied safely. They always
 * reference null-terminated data, but the null may not nessesarily be at
 * begin[size].
 */
typedef struct
{
	const char* begin;
	size_t size;
} StringRef;

//Empty values to initialize to
const static String es_empty_string;
const static StringRef es_null_ref;

//Get a pointer to the string.
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
StringRef es_slice(StringRef ref, size_t offset, size_t size);

//Make a string slice, reusing the buffer
String es_slices(String str, size_t offset, size_t size);

// Concatenate 2 strings.
String es_cat(StringRef str1, StringRef str2);

// Read up to a delimiting character from the FILE*. Adds null terminator
String es_readline(FILE* stream, char delim, size_t max);
//Note that the returned string may have max+1 characters, if max is reached,
//for the null terminator.

String es_readanyline(FILE* stream, char delim);
