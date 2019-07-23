































#ifdef WIN32
#include "config_win32.h"
#else
#include "config.h"
#endif
#include <stdlib.h>

#include "oggz_dlist.h"
#include "oggz_macros.h"

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

  OggzDList *dlist;
  OggzDListElem *dummy_front, *dummy_back;
  
  dlist = oggz_malloc(sizeof(OggzDList));
  if (dlist == NULL) return NULL;

  dummy_front = oggz_malloc(sizeof(OggzDListElem));
  if (dummy_front == NULL) {
    oggz_free (dlist);
    return NULL;
  }

  dummy_back = oggz_malloc(sizeof(OggzDListElem));
  if (dummy_back == NULL) {
    oggz_free (dummy_front);
    oggz_free (dlist);
    return NULL;
  }

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
    oggz_free(p->prev);
  }

  oggz_free(dlist->tail);
  oggz_free(dlist);

}

int
oggz_dlist_is_empty(OggzDList *dlist) {
  return (dlist->head->next == dlist->tail);
}

int
oggz_dlist_append(OggzDList *dlist, void *elem) {

  OggzDListElem *new_elem;

  if (dlist == NULL) return -1;

  new_elem = oggz_malloc(sizeof(OggzDListElem));
  if (new_elem == NULL) return -1;

  new_elem->data = elem;
  new_elem->next = dlist->tail;
  new_elem->prev = dlist->tail->prev;
  new_elem->prev->next = new_elem;
  new_elem->next->prev = new_elem;

  return 0;
}

int
oggz_dlist_prepend(OggzDList *dlist, void *elem) {

  OggzDListElem *new_elem;

  if (dlist == NULL) return -1;

  new_elem = oggz_malloc(sizeof(OggzDListElem));
  if (new_elem == NULL) return -1;

  new_elem->data = elem;
  new_elem->prev = dlist->head;
  new_elem->next = dlist->head->next;
  new_elem->prev->next = new_elem;
  new_elem->next->prev = new_elem;

  return 0;
}

int
oggz_dlist_iter(OggzDList *dlist, OggzDListIterFunc func) {

  OggzDListElem *p;

  for (p = dlist->head->next; p != dlist->tail; p = p->next) {
    int r = func(p->data);
    if (r == DLIST_ITER_ERROR) {
      return -1;
    }

    if (r == DLIST_ITER_CANCEL) {
      break;
    }
  }

  return 0;
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

int
oggz_dlist_deliter(OggzDList *dlist, OggzDListIterFunc func) {

  OggzDListElem *p, *q;
  int result = 0;

  for (p = dlist->head->next; p != dlist->tail; p = q) {
    int r = func(p->data);
    if (r == DLIST_ITER_ERROR) {
      result = -1;
    }

    if (r == DLIST_ITER_CANCEL) {
      break;
    }

    q = p->next;
    p->prev->next = p->next;
    p->next->prev = p->prev;

    oggz_free(p);
  }
  return result;
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

    oggz_free(p);
  }

}
