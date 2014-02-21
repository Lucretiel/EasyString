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
 *  clear'd or string_free'd, no matter what, before going out of scope.
 *
 *  The basic design is trying to emulate, as much as possible, the C++
 *  reference model. There are 3 kinds of constructor- copy, move, and temp.
 *  Copy constructors full copy the string and make a new one. move constructors
 *  transfer the ownership of allocated memory to the new string, nulling the
 *  pointers of the old one. temp constructors create tempstrings, which share
 *  data pointers with their source but don't own the string. These
 *  constructors have both String and char* versions.
 *
 *
 *  Implementation details:
 *
 *  There are essentially 3 kinds of strings to think about- shortstrings,
 *  regular strings, and tempstrings. They have similar but not identical
 *  behaviors over all these operations, though the library is designed so that
 *  these details are unimportant.
 *
 *  shortstrings are basically any string with less than sizeof(char*) * 3
 *  bytes. Because allocated strings need 3 pointers to keep track of the
 *  memory, we can just cram all the bytes in that space if its small enough.
 *  shortstrings always have pure copy semantics, even when moving or temping.
 *  They always have the whole space available. They are automatically created
 *  if the size is small enough.
 *
 *  regular strings have owned, allocated memory. They are distinguished from
 *  tempstrings by having a non-null alloc pointer. They are considered to own
 *  their memory; when cleared, the memory is freed. They copy their data on
 *  a copy, transfer ownership on a copy, and send the pointer on a temp.
 *
 *  tempstrings don't own their memory, and are distinguished by having null
 *  alloc pointers. They create full, owning copies when copied, but create
 *  other tempstrings when moved or temped.
 *
 *  Note that the design is such that a string can own memory but only use an
 *  offsetted subset of that memory. This is to allow a hypothetical future
 *  design that allows for owning substrings (potentially with a move)
 */

#pragma once

/*
 * Description of struct:
 *
 * OI: CLIENTS. Use the string_cstr, string_cstrc, string_size, and
 * string_available instead of accessing the struct directly.
 *
 * size: the actual size of the string
 * begin: the beginning of the useful data in the string
 * alloc_begin: the alloc'd pointer. NULL if this string doesn't own the memory
 * alloc_end: the end of the allocated memory. This is set even if the string
 *   doesn't actually own the memory.
 * shortstr: union'd against the pointers above, this allows short strings to
 * just use the bytes that would be filled with pointers, if the string is short
 * enough. It's probably 24 bytes on a 64-bit system and 12 on a 32-bit.
 */
typedef struct
{
	size_t size;
	union
	{
		struct
		{
			char* begin;
			char* alloc_begin;
			char* alloc_end;
		};
		char shortstr[3 * sizeof(char*)];
	};
} String;

//Get a pointer to the string.
char* string_cstr(String* str);
const char* string_cstrc(const String* str);

/*
 * Actual size of a string. Pass this along with a char* to other functions.
 */
size_t string_size(const String* str);

/*
 * Maximum hypothetical size of a string. This is how many bytes are available
 * to write. Feel free to mutate, using the string_cstr pointer, but make sure
 * you update size appropriately, and be aware that other strings MAY refer to
 * this memory.
 */
size_t string_avalable(const String* str);

/*
 * True if the string owns its memory. This means that when the string is
 * clear'd, the memory will become invalid; any tempstrings (possibly including
 * substrings) will become invalid.
 */
int string_owns(const String* str);

//Empty string to initialize to.
const static String empty_string;

//I'd rather not expose this, but it's useful for clients to know
//Largest size a shortstring can be
const static size_t shortstr_max = sizeof(empty_string.shortstr);

/*
 * Free the string's resources. Leaves the string in an invalid state; use it
 * before going out of scope
 */
void string_free(String* str);

//Clear the string
static inline void clear(String* str)
{ string_free(str); *str = empty_string; }

//Construction- copy, move, temp
//Make new string from cstring
String string_copy_cstr(const char* str);
String string_copyn_cstr(const char* str, size_t size);

//Make new string the owner of a cstring
String string_move_cstr(char* str);
String string_moven_cstr(char* str, size_t size);

//Use this to indicate a difference between the allocated size and data size
String string_movena_cstr(char* str, size_t size, size_t alloc);

//Make new temp string from cstring.
String string_temp_cstr(char* str);
String string_tempn_cstr(char* str, size_t size);

//Copy a string
String string_copy(const String* str);

//Move a string
String string_move(String* str);

//Make a temp string
String string_temp(String* str);

//Make an empty string with reserved memory
String string_reserved(size_t size);

/*
 * No assignment operators are provided- they're too complicated. Clear
 * appropriately and use the constructors if you need assignment.
 */

/*
 * Concatenate 2 strings. Automatically use an already allocated buffer, if
 * possible.
 */
String string_concat(String str1, String str2);

//Append one string to another
void string_append(String* str1, String str2);

/*
 * Get a substring. Be careful with this- it may or may not refer to the
 * original memory.
 *
 * If length is too long, the substring is automatically cut down to the
 * correct length
 */
String string_substr(String* str, size_t offset, size_t length);

//Compare strings
int string_compare(String str1, String str2);
int string_prefix(String str1, String str2);


