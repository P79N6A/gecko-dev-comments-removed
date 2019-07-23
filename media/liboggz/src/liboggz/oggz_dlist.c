































#ifdef WIN32
#include "config_win32.h"
#else
#include "config.h"
#endif
#include <stdlib.h>

#include "oggz_dlist.h"

typedef struct OggzDListElem {
  struct OggzDListElem  * next;
  struct OggzDListElem  * prev;
  void                  * data;
} OggzDListElem;

struct _OggzDList {
  OggzDListElem * head;
  OggzDListElem * tail;
};

OggzDList *
oggz_dlist_new (void) {

  OggzDList *dlist = malloc(sizeof(OggzDList));

  OggzDListElem * dummy_front = malloc(sizeof(OggzDListElem));
  OggzDListElem * dummy_back = malloc(sizeof(OggzDListElem));
  
  dummy_front->next = dummy_back;
  dummy_front->prev = NULL;

  dummy_back->prev = dummy_front;
  dummy_back->next = NULL;

  dlist->head = dummy_front;
  dlist->tail = dummy_back;

  return dlist;

}

void
oggz_dlist_delete(OggzDList *dlist) {

  OggzDListElem *p;

  for (p = dlist->head->next; p != NULL; p = p->next) {
    free(p->prev);
  }

  free(dlist->tail);
  free(dlist);

}

int
oggz_dlist_is_empty(OggzDList *dlist) {
  return (dlist->head->next == dlist->tail);
}

void
oggz_dlist_append(OggzDList *dlist, void *elem) {

  OggzDListElem *new_elem = malloc(sizeof(OggzDListElem));

  new_elem->data = elem;
  new_elem->next = dlist->tail;
  new_elem->prev = dlist->tail->prev;
  new_elem->prev->next = new_elem;
  new_elem->next->prev = new_elem;
}

void
oggz_dlist_prepend(OggzDList *dlist, void *elem) {

  OggzDListElem *new_elem = malloc(sizeof(OggzDListElem));

  new_elem->data = elem;
  new_elem->prev = dlist->head;
  new_elem->next = dlist->head->next;
  new_elem->prev->next = new_elem;
  new_elem->next->prev = new_elem;

}

void
oggz_dlist_iter(OggzDList *dlist, OggzDListIterFunc func) {

  OggzDListElem *p;

  for (p = dlist->head->next; p != dlist->tail; p = p->next) {
    if (func(p->data) == DLIST_ITER_CANCEL) {
      break;
    }
  }

}

void
oggz_dlist_reverse_iter(OggzDList *dlist, OggzDListIterFunc func) {

  OggzDListElem *p;

  for (p = dlist->tail->prev; p != dlist->head; p = p->prev) {
    if (func(p->data) == DLIST_ITER_CANCEL) {
      break;
    }
  }
}

void
oggz_dlist_deliter(OggzDList *dlist, OggzDListIterFunc func) {

  OggzDListElem *p, *q;

  for (p = dlist->head->next; p != dlist->tail; p = q) {
    if (func(p->data) == DLIST_ITER_CANCEL) {
      break;
    }

    q = p->next;
    p->prev->next = p->next;
    p->next->prev = p->prev;

    free(p);
  }

}

void
oggz_dlist_reverse_deliter(OggzDList *dlist, OggzDListIterFunc func) {

  OggzDListElem *p, *q;

  for (p = dlist->tail->prev; p != dlist->head; p = q) {
    if (func(p->data) == DLIST_ITER_CANCEL) {
      break;
    }

    q = p->prev;
    p->prev->next = p->next;
    p->next->prev = p->prev;

    free(p);
  }

}
