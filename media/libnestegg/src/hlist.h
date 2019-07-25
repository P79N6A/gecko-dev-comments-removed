













#ifndef _LIBP_HLIST_H_
#define _LIBP_HLIST_H_

#include <assert.h>
#include "macros.h"  




typedef struct hlist_head  hlist_head_t;
typedef struct hlist_item  hlist_item_t;




struct hlist_head
{
	hlist_item_t * next;
};

struct hlist_item
{
	hlist_item_t * next;
	hlist_item_t ** prev;
};




struct hlist_item hlist_null;




#define __hlist_init(h)      { &hlist_null }
#define __hlist_init_item(i) { &hlist_null, &(i).next }

static_inline void hlist_init(hlist_head_t * h);
static_inline void hlist_init_item(hlist_item_t * i);











static_inline void hlist_add(hlist_head_t * h, hlist_item_t * i);




static_inline void hlist_del(hlist_item_t * i);

static_inline void hlist_relink(hlist_item_t * i);
static_inline void hlist_relink_head(hlist_head_t * h);

#define hlist_for_each(i, h) \
	for (i = (h)->next; i != &hlist_null; i = i->next)

#define hlist_for_each_safe(i, tmp, h) \
	for (i = (h)->next, tmp = i->next; \
	     i!= &hlist_null; \
	     i = tmp, tmp = i->next)




static_inline void hlist_init(hlist_head_t * h)
{
	assert(h);
	h->next = &hlist_null;
}

static_inline void hlist_init_item(hlist_item_t * i)
{
	assert(i);
	i->prev = &i->next;
	i->next = &hlist_null;
}

static_inline void hlist_add(hlist_head_t * h, hlist_item_t * i)
{
	hlist_item_t * next;
	assert(h && i);
	
	next = i->next = h->next;
	next->prev = &i->next;
	h->next = i;
	i->prev = &h->next;
}

static_inline void hlist_del(hlist_item_t * i)
{
	hlist_item_t * next;
	assert(i);

	next = i->next;
	next->prev = i->prev;
	*i->prev = next;
	
	hlist_init_item(i);
}

static_inline void hlist_relink(hlist_item_t * i)
{
	assert(i);
	*i->prev = i;
	i->next->prev = &i->next;
}

static_inline void hlist_relink_head(hlist_head_t * h)
{
	assert(h);
	h->next->prev = &h->next;
}

#endif

