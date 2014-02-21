/*
 * good_string.c
 *
 *  Created on: Feb 19, 2014
 *      Author: nathan
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "easy_string.h"

//True if the string is using the shortstring optimization
const static inline bool shortstring_optimized(const String* str)
{
	return str->size < sizeof(str->shortstr);
}

//True if the string owns to allocated memory
static inline bool allocated(const String* str)
{
	return !shortstring_optimized(str) && str->owns;
}

//If the size is too long for a shortstring, allocate. Also set the size.
static inline void autoalloc(String* str, size_t size)
{
	str->size = size;

	if(!shortstring_optimized(str))
	{
		str->begin = calloc(size, 1);
		str->owns = true;
	}
}

#define STRING_CSTR(STR) shortstring_optimized(STR) ? STR->shortstr : STR->begin
char* string_cstr(String* str)
{
	return STRING_CSTR(str);
}

const char* string_cstrc(const String* str)
{
	return STRING_CSTR(str);
}

void string_free(String* str)
{
	if(allocated(str))
		free(str->begin);
}

String string_copy_cstr(const char* str)
{ return string_copyn_cstr(str, strlen(str)); }

String string_copyn_cstr(const char* str, size_t size)
{
	String result = empty_string;
	autoalloc(&result, size);
	memcpy(string_cstr(&result), str, size);
	return result;
}

String string_move_cstr(char* str)
{ return string_moven_cstr(str, strlen(str)); }

String string_moven_cstr(char* str, size_t size)
{
	String result = empty_string;
	result.size = size;

	if(shortstring_optimized(&result))
	{
		memcpy(result.shortstr, str, size);
		free(str);
	}
	else
	{
		result.begin = str;
		result.owns = 1;
	}
	return result;
}

String string_temp_cstr(char* str)
{ return string_tempn_cstr(str, strlen(str)); }

String string_tempn_cstr(char* str, size_t size)
{
	String result = empty_string;
	result.size = size;

	if(shortstring_optimized(&result))
	{
		memcpy(result.shortstr, str, size);
	}
	else
	{
		result.begin = str;
	}

	return result;
}

String string_copy(const String* str)
{
	return string_copyn_cstr(string_cstrc(str), str->size);
}

String string_move(String* str)
{
	String result = *str;
	if(!shortstring_optimized(str))
	{
		*str = empty_string;
	}
	return result;
}

String string_temp(String* str)
{
	String result = *str;
	if(!shortstring_optimized(str))
	{
		result.owns = 0;
	}
	return result;
}

String string_concat(String str1, String str2)
{
	String result = empty_string;
	autoalloc(&result, str1.size + str2.size);
	char* dest = string_cstr(&result);
	memcpy(dest, string_cstr(&str1), str1.size);
	memcpy(dest + str1.size, string_cstr(&str2), str2.size);
	string_free(&str1);
	string_free(&str2);
	return result;
}

String string_substr(String* str, size_t offset, size_t length)
{
	size_t actual_length = offset > str->size ?
		0 : offset + length > str->size ?
			str->size - offset : length;

	return string_tempn_cstr(string_cstr(str), actual_length);
}

int string_compare(String str1, String str2)
{
	int result = str1.size > str2.size ?
		1 : str1.size < str2.size ?
		-1 : memcmp(string_cstr(&str1), string_cstr(&str2), str1.size);
	string_free(&str1);
	string_free(&str2);
	return result;
}

static inline int min(int x, int y) { return x < y ? x : y; }

int string_prefix(String str1, String str2)
{
	int result = memcmp(
		string_cstr(&str1),
		string_cstr(&str2),
		min(str1.size, str2.size));
	string_free(&str1);
	string_free(&str2);
	return result;
}
