#include <stdatomic.h>
#include <stdlib.h>

#include "include/vm.h"
#include "include/object.h"
#include "include/memory.h"

void *reallocate(void *pointer, size_t oldSize, size_t newSize)
{
    if (newSize == 0)
    {
        free(pointer);
        return NULL;
    }

    void *result = realloc(pointer, newSize);
    if (result == NULL)
        exit(1); // No more memory to allocate with!
    return result;
}

static void freeObject(Obj *object)
{
    switch (object->type)
    {
    case OBJ_STRING:
    {
        ObjString *string = (ObjString *)object;             // Convert to slorp string
        FREE_ARRAY(char, string->chars, string->length + 1); // +1 because of \0
        FREE(ObjString, object);
    }
    }
}

void freeObjects()
{
    Obj *object = vm.objects;
    while (object != NULL)
    {
        Obj *next = object->next;
        freeObject(object); // free memory
        object = next;      // advance
    }
}
