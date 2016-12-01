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
#include <stdbool.h>
#include <stdarg.h>
#include "easy_string.h"

//Maximum length of a shortstring.
const static size_t shortstring_max = sizeof(es_empty_string.shortstr) - 1;

//True if a string of this length is shortstring optimized
static inline bool shortstring(size_t size)
{ return size <= shortstring_max; }

//True if this string is shortstring optimized
static inline bool shortstring_optimized(const String* str)
{ return shortstring(str->size); }

static inline size_t word_round(size_t size)
{ return (size + sizeof(void*) - 1) & ~(sizeof(void*) - 1); }
//TODO: check for OOM
/*
 * Allocate memory into a string, taking into account shortstrings. The
 * string will have at least enough space to hold a null-terminated string
 * of size `size`, This function adds a null-terminator in the correct
 * place given `size` and returns a pointer to the memory. This function
 * completely overwrites `str`.
 *
 * `hint` primarily exists to support append operations, allowing capacity
 * to be doubled on a reallocation, even if string length isn't. The
 * function will try to allocate `hint` bytes, but may shortstring optimize
 * and will always allocate at least enough for `size`.
 */
static inline char* autoalloc(String* str, size_t size, size_t hint)
{
	str->size = size;
	char* mem = str->shortstr;
	if(!shortstring(size))
	{
		size_t alloc_amount = word_round(hint > size ? hint : size + 1);
		mem = str->begin = malloc(alloc_amount);
		str->alloc_size = alloc_amount;
	}
	mem[size] = '\0';
	return mem;
}

//Total available allocated size, including that nul terminator
static inline size_t available(const String* str)
{
	return shortstring_optimized(str) ?
		sizeof(str->shortstr) :
		str->alloc_size;
}

char* es_cstr(String* str)
{ return shortstring_optimized(str) ? str->shortstr : str->begin; }

const char* es_cstrc(const String* str)
{ return shortstring_optimized(str) ? str->shortstr : str->begin; }

void es_free(String* str)
{ if(!shortstring_optimized(str)) free(str->begin); }

String es_copy(StringRef str)
{
	String result;
	memcpy(autoalloc(&result, str.size, 0), str.begin, str.size);
	return result;
}

/*
 * Note that the move methods are pretty much the only way to create a string
 * without a null terminator.
 */
String es_move(String* str)
{
	String result = *str;
	if(!shortstring_optimized(str))
		*str = es_empty_string;
	return result;
}

String es_move_cstrn(char* str, size_t size)
{
	String result;
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
		result.alloc_size = size;
	}
	return result;
}

/*
 * Quick recap of C variadic functions:
 *
 * va_list args;
 *
 * va_start(args, last_arg_before_variadic);
 * TYPE next = va_get(args, TYPE); //Repeat this over and over
 * va_end(args);
 *
 * Obviously don't call va_get too many times, but you can do it 0 to n times
 * between va_start and va_end. You are also welcome to do another va_start
 * after a previous va_end to start over.
 */
String es_printf(const char* format, ...)
{
	va_list args;

	//TODO: try an initial shortstr printf here.
	//Get the number of bytes required.
	va_start(args, format);
	int size = vsnprintf(0, 0, format, args);
	va_end(args);

	//If empty, or an error, return empty_string
	if(size <= 0) return es_empty_string;
	//TODO: find a way to report errors

	String result;

	va_start(args, format);
	vsprintf(autoalloc(&result, size, 0), format, args);
	va_end(args);

	return result;
}

StringRef es_ref(const String* str)
{ return es_tempn( ES_STRCNSTSIZE(str) ); }

StringRef es_tempn(const char* str, size_t size)
{
	StringRef result = { (str && size ? str : ""), size };
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
	return es_tempn(ref.begin + offset, size);
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
		memcpy(str->shortstr, ptr + offset, size);
		str->shortstr[size] = '\0';
		free(ptr);
	}

	//Memmove otherwise. There's no way to go from shortstring to normal,
	//and this covers "shortstr to shortstr" and "str to str"
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
	char* dest = autoalloc(&result, str1.size + str2.size, 0);
	memcpy(dest, str1.begin, str1.size);
	memcpy(dest + str1.size, str2.begin, str2.size);
	return result;
}

void es_append(String* str1, const StringRef str2)
{
	//Do nothing if str2 is empty.
	if(str2.size == 0) return;

	size_t final_size = str1->size + str2.size;

	//If necessary, reallocate str1
	if(available(str1) < final_size + 1)
	{
		String result;

		//Wisdom says: allocate geometrically when reallocating
		char* mem = autoalloc(&result, final_size, (available(str1) * 3) / 2);

		//Copy str1 into the new string.
		memcpy(mem, ES_STRCNSTSIZE(str1));

		//Perform the append
		memcpy(mem + str1->size, ES_STRREFSIZE(&str2));

		//Free and reset str1
		es_free(str1);
		*str1 = result;
	}

	//If str1 is already big enough, append directly into it
	else
	{
		char* mem = es_cstr(str1);
		memcpy(mem + str1->size, ES_STRREFSIZE(&str2));
		mem[final_size] = '\0';
		str1->size = final_size;
	}
}

String es_tolower(StringRef str)
{
	String result = es_empty_string;
	char* copy = autoalloc(&result, str.size, 0);
	for(size_t i = 0; i < str.size; ++i)
		copy[i] = tolower(str.begin[i]);
	return result;
}

static inline int char_value(char c)
{
	return c < '0' || c > '9' ? -1 : c - '0';
}

//TODO: meaningful error codes (overflow, no digits, etc)
int es_toul(unsigned long* result, StringRef str)
{
	unsigned long count = 0;
	int i;

	for(i = 0; i < str.size; ++i)
	{
		//Get digit value
		int decimal = char_value(str.begin[i]);

		//Stop loop if non-digit
		if(decimal == -1) break;

		//Store old count, shift current count
		const unsigned long old_count = count;
		count *= 10;

		//Check for overflow
		if(count / 10 != old_count) return 1;

		//Add digit
		count += decimal;

		//Check for overflow agai
		if(count < old_count) return 1;
	}
	//If we didn't even get a single loop, it's an error
	if(i == 0) return 1;
	*result = count;
	return 0;
}

int es_sizecmp(size_t str1, size_t str2)
{
	return str1 < str2 ? -1 : str1 > str2 ? 1 : 0;
}

int es_compare(StringRef str1, StringRef str2)
{
	int cmp = es_prefix(str1, str2);
	return cmp ? cmp : es_sizecmp(str1.size, str2.size);
}

int es_prefix(StringRef str1, StringRef str2)
{
	return memcmp(str1.begin, str2.begin, min_size(str1.size, str2.size));
}

//TODO: reimplement to make use of implementation details of str1.
const static size_t buffer_size = 4096;
String es_readline(FILE* stream, char delim, size_t max)
{
	String result = es_empty_string;

	if(ferror(stream) || feof(stream)) return result;

	int c = -1;
	do
	{
		//Buffer to read into
		char buffer[buffer_size];

		//Read in up to buffer_size bytes or until max is reached
		size_t amount_read = 0;
		for(int i = 0; i < buffer_size && max; ++i)
		{
			//Read byte
			c = getc(stream);

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

