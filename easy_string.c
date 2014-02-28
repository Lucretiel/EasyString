/*
 * easy_string.c
 *
 *  Created on: Feb 19, 2014
 *      Author: nathan
 */

#include <string.h> //String ops. copy, cmp, etc
#include <stdlib.h> //Allocation
#include <stdint.h> //SIZE_MAX
#include <ctype.h> //tolower
#include "easy_string.h"

const static size_t shortstring_max = sizeof(es_empty_string.shortstr) - 1;

//True if a string of this length is shortstring optimized
static inline int shortstring(size_t size)
{ return size <= shortstring_max; }

//True if this string is shortstring optimized
static inline int shortstring_optimized(const String* str)
{ return shortstring(str->size); }

//TODO: check for OOM
//Allocate memory to a string, taking into account shortstrings
//Return the char* to the buffer
static inline char* autoalloc(String* str, size_t size)
{
	str->size = size;
	if(!shortstring(size))
	{
		char* mem = malloc(size + 1);
		str->begin = mem;
		str->alloc_end = mem + size + 1;
		mem[size] = '\0';
		return mem;
	}
	else
	{
		return str->shortstr;
	}
}

static inline size_t available(const String* str)
{
	return shortstring_optimized(str) ?
		sizeof(str->shortstr) :
		str->alloc_end - str->begin;
}

char* es_cstr(String* str)
{ return shortstring_optimized(str) ? str->shortstr : str->begin; }

const char* es_cstrc(const String* str)
{ return shortstring_optimized(str) ? str->shortstr : str->begin; }

void es_free(String* str)
{ if(!shortstring_optimized(str)) free(str->begin); }

String es_copy(StringRef str)
{
	String result = es_empty_string;
	char* mem = autoalloc(&result, str.size);
	memcpy(mem, str.begin, str.size);
	return result;
}

String es_move(String* str)
{
	String result = *str;
	if(!shortstring_optimized(str))
		*str = es_empty_string;
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
		result.shortstr[size] = '\0';
		free(str);
	}
	else
	{
		result.begin = str;
		result.alloc_end = str + size;
	}
	return result;
}

StringRef es_ref(const String* str)
{ return es_tempn( ES_STRCNSTSIZE(str) ); }

StringRef es_temp(const char* str)
{ return es_tempn(str, strlen(str) ); }

StringRef es_tempn(const char* str, size_t size)
{
	StringRef result = {str, size};
	return result;
}

static inline size_t min_size(size_t x, size_t y)
{ return x < y ? x : y; }

static inline void update_slice_indexes(size_t str_size, size_t* offset,
	size_t* size)
{
	if(*offset >= str_size)
		*size = 0;
	else
		*size = min_size(*size, str_size - *offset);
}


StringRef es_slice(StringRef ref, size_t offset, size_t size)
{
	update_slice_indexes(ref.size, &offset, &size);
	if(size == 0)
		return es_null_ref;

	StringRef result = {ref.begin + offset, size};
	return result;
}

void es_slices(String* str, size_t offset, size_t size)
{
	update_slice_indexes(str->size, &offset, &size);

	//Just clear the string if the result will be empty
	if(size == 0)
	{
		es_clear(str);
		return;
	}

	//Copy to shortstring memory if it's becoming a shortstring
	else if (shortstring(size) && !shortstring_optimized(str))
	{
		char* ptr = str->begin;
		memcpy(str->shortstr, str->begin + offset, size);
		str->shortstr[size] = '\0';
		free(ptr);
	}

	//Memmove otherwise
	else
	{
		char* mem = es_cstr(str);
		memmove(mem, mem + offset, size);
		mem[size] = '\0';
	}

	str->size = size;
}

String es_cat(StringRef str1, StringRef str2)
{
	String result = es_empty_string;
	char* dest = autoalloc(&result, str1.size + str2.size);
	memcpy(dest, str1.begin, str1.size);
	memcpy(dest + str1.size, str2.begin, str2.size);
	return result;
}

void es_append(String* str1, StringRef str2)
{
	size_t final_size = str1->size + str2.size;

	if(final_size + 1 < available(str1))
	{
		String result = es_cat(es_ref(str1), str2);
		es_free(str1);
		*str1 = result;
	}
	else
	{
		char* mem = es_cstr(str1);
		memcpy(mem + str1->size, str2.begin, str2.size);
		mem[final_size] = '\0';
		str1->size = final_size;
	}
}

String es_tolower(StringRef str)
{
	String result = es_empty_string;
	char* copy = autoalloc(&result, str.size);
	for(size_t i = 0; i < str.size; ++i)
		copy[i] = tolower(str.begin[i]);
	return result;
}

static inline int char_value(char c)
{
	return c < '0' || c > '9' ? -1 : c - '0';
}

int es_toul(unsigned long* result, StringRef str)
{
	unsigned long count = 0;
	int i;

	for(i = 0; i <= str.size; ++i)
	{
		int decimal = char_value(str.begin[i]);
		if(decimal == -1) break;
		const unsigned long old_count = count;
		count *= 10;
		//TODO: potential uncaught overflow?
		if(count < old_count) return 1;
		count += decimal;
		if(count < old_count) return 1;
	}
	if(i == 0) return 1;
	*result = count;
	return 0;
}

int es_sizecmp(size_t str1, size_t str2)
{ return str1 < str2 ? -1 : str1 > str2 ? 1 : 0; }

int es_compare(StringRef str1, StringRef str2)
{
	int sizecmp = es_sizecmp(str1.size, str2.size);
	if(sizecmp) return sizecmp;
	else return memcmp(str1.begin, str2.begin, str1.size);
}

int es_prefix(StringRef str1, StringRef str2)
{
	return memcmp(str1.begin, str2.begin, min_size(str1.size, str2.size));
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
		es_append(&result, es_tempn(buffer, amount_read));

	//Repeat until Error, delimiter found, or max reached.
	} while(c != EOF && c != delim && max);

	return result;
}

String es_readanyline(FILE* stream, char delim)
{ return es_readline(stream, delim, SIZE_MAX); }

