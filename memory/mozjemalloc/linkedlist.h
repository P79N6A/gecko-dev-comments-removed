































#ifndef linkedlist_h__
#define linkedlist_h__

#include <stddef.h>

typedef struct LinkedList_s LinkedList;

struct LinkedList_s {
	LinkedList *next;
	LinkedList *prev;
};


#define LinkedList_Get(e, type, prop) \
  (type*)((char*)(e) - offsetof(type, prop))


void LinkedList_InsertHead(LinkedList *l, LinkedList *e)
{
	e->next = l;
	e->prev = l->prev;
	e->next->prev = e;
	e->prev->next = e;
}

void LinkedList_Remove(LinkedList *e)
{
	e->prev->next = e->next;
	e->next->prev = e->prev;
	e->next = e;
	e->prev = e;
}

bool LinkedList_IsEmpty(LinkedList *e)
{
	return e->next == e;
}

void LinkedList_Init(LinkedList *e)
{
	e->next = e;
	e->prev = e;
}

#endif
