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

static inline bool shortstr(size_t size)
{
	return size <= shortstr_max;
}

static inline bool shortstr_optimized(const String* str)
{
	return shortstr(str->size);
}

/*
 * Get the total amount of allocated memory, including potential space before
 * begin. Returns a size only if the string owns allocated memory, otherwise
 * returns 0.
 */
static inline size_t allocated_size(const String* str)
{
	return shortstr_optimized(str) ?
		0 : str->alloc_begin ?
			str->alloc_end - str->alloc_begin :
			0;
}

/*
 * Force str to have at least size bytes available, possibly with a malloc.
 * This is an unsafe function; make sure to set everything else correctly
 * beforehand (freeing pointers, etc).
 *
 * Note that a side-effect of this design is that a string of
 * size < shortstr_max can't have allocated memory. This is fine.
 */
static inline void autoalloc(String* str, size_t size)
{
	str->size = size;
	if(!shortstr(size))
	{
		str->alloc_begin = malloc(size);
		str->alloc_end = str->alloc_begin + size;
		str->begin = str->alloc_begin;
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
#undef STRING_CSTR

size_t string_size(const String* str)
{
	return str->size;
}

size_t string_available(const String* str)
{
	return shortstr_optimized(str) ?
		sizeof(str->shortstr) :
		str->alloc_end - str->begin;
}

int string_owns(const String* str)
{
	return shortstr_optimized(str) || str->alloc_begin;
}

void string_free(String* str)
{
	if(allocated_size(str))
		free(str->alloc_begin);
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
{ return string_movena_cstr(str, size, size); }

String string_movena_cstr(char* str, size_t size, size_t alloc)
{
	String result = empty_string;
	result.size = size;

	if(shortstr(size))
	{
		memcpy(result.shortstr, str, size);
		free(str);
	}
	else
	{
		result.begin = str;
		result.alloc_begin = str;
		result.alloc_end = str + alloc;
	}
	return result;
}

String string_temp_cstr(char* str)
{ return string_tempn_cstr(str, strlen(str)); }

String string_tempn_cstr(char* str, size_t size)
{
	String result = empty_string;
	result.size = size;

	if(shortstr(size))
	{
		memcpy(result.shortstr, str, size);
	}
	else
	{
		result.begin = str;
		result.alloc_end = str + size;
	}

	return result;
}

String string_copy(const String* str)
{ return string_copyn_cstr(string_cstrc(str), str->size); }

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
		result.alloc_begin = 0;
	}
	return result;
}

String string_reserved(size_t size)
{
	String result = empty_string;
	autoalloc(&result, size);
	return result;
}

String string_concat(String str1, String str2)
{
	String result = empty_string;
	result.size = string_size(&str1) + string_size(&str2);
	//Attempt to just do buffer pointer shifts.

	/*
	 * We can't do a pointer shift if the 2 string refer to the same memory, and
	 * both have an offset. We can only do it if str2 owns its data, or str1
	 * doesn't have an offset, or if str2 happens to be at the correct offset.
	 */
	if(allocated_size(&str1) >= result.size &&
		(string_owns(&str2) ||
			str1.begin == str1.alloc_begin ||
			str2.begin == str1.alloc_begin + str1.size ))
	{
		//Give str1's buffer to result
		result.alloc_begin = str1.alloc_begin;
		result.alloc_end = str1.alloc_end;
		result.begin = result.alloc_begin;

		//Shift the memory to the beginning of the allocated space, if needed
		memmove(result.alloc_begin, str1.begin, str1.size);

		//Copy str2 to the array
		memcpy(result.alloc_begin + str1.size, string_cstrc(&str2), str2.size);

		//Wipe str1's pointers so it isn't free'd
		str1.alloc_begin = 0;
	}
	/*
	 * Same logic and above. Need to be sure it's safe to memmove and memcpy.
	 */
	else if(allocated_size(&str2) >= result.size &&
		(string_owns(&str1) ||
			str1.begin == str2.alloc_begin ||
			str2.begin == str2.alloc_begin + str1.size))
	{
		//Give str2's buffer to result
		result.alloc_begin = str2.alloc_begin;
		result.alloc_end = str2.alloc_end;
		result.begin = result.alloc_begin;

		//Shift str2's memory over, to make room for str1
		memmove(result.alloc_begin + str1.size, str2.begin, str2.size);

		//Copy str1 to the array
		memcpy(result.alloc_begin, string_cstrc(&str1), str1.size);

		//Wipe str2's pointers so it isn't free'd
		str2.alloc_begin = 0;
	}
	else
	{
		/*
		 * Allocate new memory, copy both strings. Can't use raw underlying
		 * pointers because this may be a shortstring.
		 */
		autoalloc(&result, result.size);
		char* dest = string_cstr(&result);
		memcpy(dest, string_cstr(&str1), str1.size);
		memcpy(dest + str1.size, string_cstr(&str2), str2.size);
	}
	string_free(&str1);
	string_free(&str2);
	return result;
}

void string_append(String* dest, String source)
{
	*dest = string_concat(string_move(dest), source);
}

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

String string_substr(String str, size_t offset, size_t length)
{
	String result;

	if(offset < string_size(str))
	{
		result.size = MIN(length, string_size(str) - offset);
		if(shortstr(length))
		{
			memcpy(result.shortstr, string_cstrc(&str), result.size);
		}
		else
		{
			result.begin = string_cstr(&str) + result.size;
			result.alloc_begin = str.alloc_begin;
			result.alloc_end = str.alloc_end;
			str.alloc_begin = 0;
		}
	}
	else
	{
		result = empty_string;
	}
	string_free(str);
	return result;
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
