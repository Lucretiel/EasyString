/*
 * good_string.c
 *
 *  Created on: Feb 19, 2014
 *      Author: nathan
 */

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "easy_string.h"

const static size_t shortstring_max = sizeof(es_empty_string.shortstr) - 1;

//True if a string of this length is shortstring optimized
static inline int shortstring(size_t size)
{ return size <= shortstring_max; }

//True if this string is shortstring optimized
static inline int shortstring_optimized(const String* str)
{ return shortstring(str->size); }

//Allocate memory to a string, taking into account shortstrings
//Return the char* to the buffer
static inline char* autoalloc(String* str, size_t size)
{
	str->size = size;
	if(!shortstring(size))
	{
		char* mem = calloc(size + 1, 1);
		str->begin = mem;
		return mem;
	}
	else
	{
		return str->shortstr;
	}
}

const char* es_cstr(const String* str)
{ return shortstring_optimized(str) ? str->shortstr : str->begin; }

void es_free(String* str)
{ if(!shortstring_optimized(str)) free(str->begin); }

String es_copy(StringRef str)
{ return es_copy_cstrn(ES_STRREFSIZE(&str)); }

String es_copy_cstr(const char* str)
{ return es_copy_cstrn(str, strlen(str)); }

String es_copy_cstrn(const char* str, size_t size)
{
	String result = es_empty_string;
	char* mem = autoalloc(&result, size);
	memcpy(mem, str, size);
	return result;
}

String es_move(String* str)
{
	String result = *str;
	if(!shortstring_optimized(&str))
		str = es_empty_string;
	return result;
}

String es_move_cstr(char* str)
{ return es_move_cstrn(str, strlen(str)); }

String es_move_cstrn(char* str, size_t size)
{
	String result = es_empty_string;
	result.size = size;
	if(shortstring(size))
	{
		memcpy(result.shortstr, str, size);
		free(str);
	}
	else
	{
		result.begin = str;
	}
	return result;
}

StringRef es_ref(const String* str)
{ return es_temp_cstrn( ES_STRINGSIZE(str)); }

StringRef es_temp(const char* str)
{ return es_temp_cstr(str, strlen(str)); }

StringRef es_tempn(const char* str, size_t size)
{
	StringRef result = {str, size};
	return result;
}

static inline size_t min_size(size_t x, size_t y)
{ return x < y ? x : y; }

StringRef es_slice(StringRef ref, size_t offset, size_t size)
{
	if(size <= 0 || offset >= ref.size)
		return es_null_ref;

	StringRef result = {ref.begin + offset, min_size(size, ref.size - offset)};
	return result;
}

String es_slices(String str, size_t offset, size_t size)
{
	//Get the actual size, without overflowing str.size
	size_t actual_size = offset >= str.size ?
		0 : min_size(size, offset - size);

	//Just clear the string if the result will be empty
	if(actual_size == 0)
	{
		es_clear(&str);
	}

	//If the slice will be a shortstring
	else if(shortstring(actual_size))
	{
		//If the string is already a shortstring, just memmove
		if(shortstring_optimized(&str))
		{
			memmove(str.shortstr, str.shortstr+offset, actual_size);
			str.shortstr[actual_size] = '\0';
		}
		//Otherwise, memcpy then free the used memory
		else
		{
			char* ptr = str.begin;
			memcpy(str.shortstr, str.begin + offset, actual_size);
			str.shortstr[actual_size] = '\0';
			free(ptr);
		}
	}
	//If the slice is not a shortstring, just memmove
	else
	{
		memmove(str.begin, str.begin + offset, actual_size);
		str.begin[actual_size] = '\0';
	}
	str.size = actual_size;
	return str;
}

String es_cat(StringRef str1, StringRef str2)
{
	String result = es_empty_string;
	char* dest = autoalloc(&result, str1.size + str2.size);
	memcpy(dest, str1.begin, str1.size);
	memcpy(dest + str1.size, str2.begin, str2.size);
	return result;
}

const static size_t buffer_size = 4096;
String es_readline(FILE* stream, char delim, size_t max)
{
	String result = es_empty_string;

	int c;
	do
	{
		//Buffer to read into
		char buffer[buffer_size];

		//Read in up to buffer_size bytes or until max is reached
		size_t amount_read = 0;
		for(int i = 0; i < buffer_size && max; ++i)
		{
			//Read byte
			c = fgetc(stream);

			//Break if EOF or error
			if(c == EOF)
				break;

			//Fill byte
			buffer[i] = c;
			++amount_read;
			--max;

			//Break if delimiter
			if(c == delim)
				break;
		}
		//Concat the bytes into the result string
		String new_result = es_cat(
			es_ref(&result),
			es_tempn(buffer, amount_read));
		es_free(result);
		result = new_result;

	//Repeat until Error, delimiter found, or max reached.
	} while(c != EOF && c != delim && max);

	return result;
}

String es_readanyline(FILE* stream, char delim)
{ return es_readline(stream, delim, SIZE_MAX); }

