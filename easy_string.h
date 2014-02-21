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
 *  These strings are NOT null terminated. This is to allow substrings to work
 *  as simple pointers.
 *
 *  Strings can be either owning strings or tempstrings. Tempstrings are like
 *  weak references; they don't free their memory when cleared. Depending on
 *  how they were produced, they may or may share their memory with an owning
 *  string; Care should be taken to ensure they don't outlive their source.
 *
 *  In general, it should be assumed that stack-scoped String objects should be
 *  cleared, no matter what, before going out of scope.
 */

#pragma once

typedef struct
{
	size_t size;
	union
	{
		struct
		{
			char* begin;
			int owns;
		};
		char shortstr[2 * sizeof(char*)];
	};
} String;

//Get a pointer to the string.
char* string_cstr(String* str);
const char* string_cstrc(const String* str);

//Empty string to initialize to.
const static String empty_string;

//Free the string's resources
void string_free(String* str);

//Clear the string
static inline void clear(String* str)
{ string_free(str); *str = empty_string; }

//Make new string from cstring
String string_copy_cstr(const char* str);
String string_copyn_cstr(const char* str, size_t size);

//Make new string the owner of a cstring
String string_move_cstr(char* str);
String string_moven_cstr(char* str, size_t size);

//Make new temp string from cstring
String string_temp_cstr(char* str);
String string_tempn_cstr(char* str, size_t size);

//Copy a string
String string_copy(const String* str);

//Move a string
String string_move(String* str);

//Make a temp string
String string_temp(String* str);

//Concat 2 strings
String string_concat(String str1, String str2);

//Get a substring
String string_substr(String* str, size_t offset, size_t length);

//Compare strings
int string_compare(String str1, String str2);
int string_prefix(String str1, String str2);


