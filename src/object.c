#include"include/object.h"

#include<stdio.h>
#include<string.h>

#include "include/memory.h"
#include "include/value.h"
#include "include/vm.h"
#include "include/object.h"
#include "include/table.h"

#define ALLOCATE_OBJ(type, objectType) \
	(type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(size_t size, ObjType type)
{
	Obj* object = (Obj*)reallocate(NULL, 0, size);
	object->type = type;

	// Insert this new object as head in the vm's pointer to objects
	object->next = vm.objects;
	vm.objects = object;
	
	return object;
}

static ObjString* allocateString(char* chars, int length, uint32_t hash)
{
	ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	string->length = length;
	string->chars = chars;
	string->hash = hash;
	tableSet(&vm.strings, string, NIL_VAL);
	return string;
}

// FNV-1a hash, also called "Fowler-Noll-Vo" function
static uint32_t hashString(const char* key, int length)
{
	uint32_t hash = 2166136261u; // 32 bit offset_basis
	for (int i = 0; i < length; i++)
	{
		hash ^= (uint8_t)key[i];
		hash *= 16777619; // 32 bit FNV prime
	}
	return hash;
}

ObjString* copyString(const char* chars, int length)
{
	uint32_t hash = hashString(chars, length);
	ObjString* interned = tableFindKey(&vm.strings, chars, length, hash);
	if (interned != NULL)
	{
		// String already exists
		return interned;
	}
	// First copy the literal string onto a newly allocated heap blob
	char* heapChars = ALLOCATE(char, length + 1);
	memcpy(heapChars, chars, length);
	heapChars[length] = '\0';
	return allocateString(heapChars, length, hash);
}

void printObject(Value value)
{
	switch (value.as.obj->type)
	{
		case OBJ_STRING:
			printf("%s", AS_CSTRING(value));
			break;
	}
}

ObjString* takeString(char* chars, int length)
{
	// Create slorp string from already existing heap blob of characters
	uint32_t hash = hashString(chars, length);
	ObjString* interned = tableFindKey(&vm.strings, chars, length, hash);
	if (interned != NULL) 
	{
		FREE_ARRAY(char, chars, length + 1);
	}
	return allocateString(chars, length, hash);
}
