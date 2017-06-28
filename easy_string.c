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

//Maximum size of a shortstring.
const static size_t shortstring_max = sizeof(es_empty_string.shortstr) - 1;

//True if a string of this length is shortstring optimized
static inline bool shortstring(size_t size)
{ return size <= shortstring_max; }

//True if this string is shortstring optimized
static inline bool shortstring_optimized(const String* str)
{ return shortstring(str->size); }


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
		size_t alloc_amount = hint > size ? hint : size + 1;
		mem = str->begin = malloc(alloc_amount);
		str->alloc_size = alloc_amount;
	}
	mem[size] = '\0';
	return mem;
}

//Total available allocated size, including that null terminator
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
	String result;

	//Get the number of bytes required. Try a shortstring print.
	va_start(args, format);
	int size = vsnprintf(result.shortstr, sizeof(result.shortstr), format, args);
	va_end(args);

	//If empty, or an error, return empty_string
	//TODO: find a way to report errors
	if(size <= 0)
		return es_empty_string;

	//If the shortstring print succeeded, use that
	else if(shortstring(size))
	{
		result.size = size;
		return result;
	}

	// Otherwise, allocate and retry
	else
	{
		va_start(args, format);
		vsprintf(autoalloc(&result, size, 0), format, args);
		va_end(args);
		return result;
	}
}

StringRef es_ref(const String* str)
{ return es_tempn( ES_STRCNSTSIZE(str) ); }

StringRef es_tempn(const char* str, size_t size)
{
	StringRef result;
	if(str && size)
	{
		result.begin = str;
		result.size = size;
	}
	else
	{
		result.begin = "";
		result.size = 0;
	}
	return result;
}

static inline size_t min_size(size_t x, size_t y)
{ return x < y ? x : y; }

static inline size_t corrected_size(size_t str_size, size_t offset,
	size_t size)
{
	return offset >= str_size ? 0 : min_size(size, str_size - offset);
}

StringRef es_slice(StringRef ref, size_t offset, size_t size)
{
	return es_tempn(ref.begin + offset, corrected_size(ref.size, offset, size));
}

void es_slices(String* str, size_t offset, size_t size)
{
	if(str->size == 0)
		return;

	size = corrected_size(str->size, offset, size);

	//Just clear the string if the result will be empty
	if(size == 0)
	{
		es_clear(str);
		return;
	}

	//Do nothing if the slice is the same size as the original string
	else if (size == str->size)
	{
		return;
	}

	//Copy to shortstring memory if it's becoming a shortstring
	else if (shortstring(size) && !shortstring_optimized(str))
	{
		char* ptr = str->begin;
		char* dst = memcpy(str->shortstr, ptr + offset, size);
		free(ptr);

		dst[size] = '\0';
		str->size = size;
		return;
	}

	// Note that for these last two blocks, we're doing either "short to short"
	// or "alloc to alloc"

	// If the offset is 0, just update the size.
	else if (offset == 0)
	{
		es_cstr(str)[size] = '\0';
		str->size = size;
		return;
	}

	//Memmove otherwise.
	else
	{
		dst = es_cstr(str);
		memmove(dst, dst + offset, size);
		dst[size] = '\0'
		str->size = size;
		return;
	}
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
	if(str2.size == 0) return;

	EsBuffer buf = es_buffer_grow(str1, str2.size + 1);

	memcpy(buf.buffer, ES_STRREFSIZE(&str2));
	buf.buffer[str2.size] = '\0'

	es_buffer_grow_commit(str2.size);
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

static inline EsBuffer compute_buffer_partial(char* begin, size_t size, size_t buf_available)
{
	EsBuffer result = {buf_available, begin + size}
	return result;
}

static inline EsBuffer compute_buffer(char* begin, size_t size, size_t available)
{
	return compute_buffer_partial(begin, size, available - size)
}

EsBuffer es_buffer(String* str)
{
	return compute_buffer(es_cstr(str), str->size, available(str))
}

EsBuffer es_buffer_grow(String* str, size_t extra)
{
	// Rationalle: it is very common for strings to be allocated with size + 1
	// == available, due to the trailing \0. We'd rather just reallocate if we
	// have a trailing null, on the assumtion that the user will want to read
	// more than one byte into the buffer.
	extra = extra ? extra : 2;

	size_t buf_available = available(str) - str->size;
	if(buf_available < extra)
		return es_buffer_force_grow(str, 0);
	else
		return compute_buffer_partial(es_cstr(str), str->size, buf_available)
}

EsBuffer es_buffer_force_grow(String* str, size_t extra)
{
	String new_str;

	char* mem = autoalloc(&new_str, str->size + extra, (available(str) * 3) / 2);

	memcpy(mem, ES_STRCNSTSIZE(str));

	es_free(str)
	*str = new_str;

	return compute_buffer(new_str.begin, new_str.size, new_str.alloc_size)
}

void es_buffer_commit(String* str, size_t amount)
{
	str->size += amount
}
