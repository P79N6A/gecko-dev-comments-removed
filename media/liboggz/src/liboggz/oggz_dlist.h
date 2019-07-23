































#ifndef __OGGZ_DLIST_H__
#define __OGGZ_DLIST_H__

struct _OggzDList;
typedef struct _OggzDList OggzDList;

typedef enum {DLIST_ITER_ERROR=-1, DLIST_ITER_CANCEL=0, DLIST_ITER_CONTINUE=1} OggzDListIterResponse;

typedef OggzDListIterResponse (*OggzDListIterFunc) (void *elem);

OggzDList *
oggz_dlist_new (void);

void
oggz_dlist_delete(OggzDList *dlist);

int
oggz_dlist_is_empty(OggzDList *dlist);

int
oggz_dlist_append(OggzDList *dlist, void *elem);

int
oggz_dlist_prepend(OggzDList *dlist, void *elem);

int
oggz_dlist_iter(OggzDList *dlist, OggzDListIterFunc func);

void
oggz_dlist_reverse_iter(OggzDList *dlist, OggzDListIterFunc func);

int
oggz_dlist_deliter(OggzDList *dlist, OggzDListIterFunc func);

void
oggz_dlist_reverse_deliter(OggzDList *dlist, OggzDListIterFunc func);

#endif
