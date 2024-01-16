#ifndef slorp_table_h
#define slorp_table_h

#include "value.h"
#include <stdint.h>

typedef struct {
	ObjString* key;
	Value value;
} Entry;

typedef struct {
	int count;
	int capacity;
	Entry* entries;
} Table;

void initTable(Table* table);
void freeTable(Table* table);
void tableAddAll(Table* from, Table* to);
bool tableSet(Table* table, ObjString* key, Value value);
bool tableGet(Table* table, ObjString* key, Value* value);
bool tableDelete(Table* table, ObjString* key);
ObjString* tableFindKey(Table* table, const char* chars, int length, uint32_t hash);

#endif
