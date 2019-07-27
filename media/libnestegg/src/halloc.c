













#include <stdlib.h>  
#include <string.h>  

#include "halloc.h"
#include "align.h"
#include "hlist.h"




typedef struct hblock
{
#ifndef NDEBUG
#define HH_MAGIC    0x20040518L
	long          magic;
#endif
	hlist_item_t  siblings; 
	hlist_head_t  children; 
	max_align_t   data[1];  
	
} hblock_t;

#define sizeof_hblock offsetof(hblock_t, data)




realloc_t halloc_allocator = NULL;

#define allocator halloc_allocator




static void _set_allocator(void);
static void * _realloc(void * ptr, size_t n);

static int  _relate(hblock_t * b, hblock_t * p);
static void _free_children(hblock_t * p);




void * halloc(void * ptr, size_t len)
{
	hblock_t * p;

	
	if (! allocator)
	{
		_set_allocator();
		assert(allocator);
	}

	
	if (! ptr)
	{
		if (! len)
			return NULL;

		p = allocator(0, len + sizeof_hblock);
		if (! p)
			return NULL;
#ifndef NDEBUG
		p->magic = HH_MAGIC;
#endif
		hlist_init(&p->children);
		hlist_init_item(&p->siblings);

		return p->data;
	}

	p = structof(ptr, hblock_t, data);
	assert(p->magic == HH_MAGIC);

	
	if (len)
	{
		p = allocator(p, len + sizeof_hblock);
		if (! p)
			return NULL;

		hlist_relink(&p->siblings);
		hlist_relink_head(&p->children);
		
		return p->data;
	}

	
	_free_children(p);
	hlist_del(&p->siblings);
	allocator(p, 0);

	return NULL;
}

void hattach(void * block, void * parent)
{
	hblock_t * b, * p;
	
	if (! block)
	{
		assert(! parent);
		return;
	}

	
	b = structof(block, hblock_t, data);
	assert(b->magic == HH_MAGIC);

	hlist_del(&b->siblings);

	if (! parent)
		return;

	
	p = structof(parent, hblock_t, data);
	assert(p->magic == HH_MAGIC);
	
	
	assert(b != p);          
	assert(! _relate(p, b)); 

	hlist_add(&p->children, &b->siblings);
}




void * h_malloc(size_t len)
{
	return halloc(0, len);
}

void * h_calloc(size_t n, size_t len)
{
	void * ptr = halloc(0, len*=n);
	return ptr ? memset(ptr, 0, len) : NULL;
}

void * h_realloc(void * ptr, size_t len)
{
	return halloc(ptr, len);
}

void   h_free(void * ptr)
{
	halloc(ptr, 0);
}

char * h_strdup(const char * str)
{
	size_t len = strlen(str);
	char * ptr = halloc(0, len + 1);
	return ptr ? (ptr[len] = 0, memcpy(ptr, str, len)) : NULL;
}




static void _set_allocator(void)
{
	void * p;
	assert(! allocator);
	
	









	allocator = realloc;
	if (! (p = malloc(1)))
		
		return;
		
	if ((p = realloc(p, 0)))
	{
		
		allocator = _realloc;
		free(p);
	}
}

static void * _realloc(void * ptr, size_t n)
{
	


	if (n)
		return realloc(ptr, n);
	free(ptr);
	return NULL;
}

static int _relate(hblock_t * b, hblock_t * p)
{
	hlist_item_t * i;

	if (!b || !p)
		return 0;

	




	hlist_for_each(i, &p->children)
	{
		hblock_t * q = structof(i, hblock_t, siblings);
		if (q == b || _relate(b, q))
			return 1;
	}
	return 0;
}

static void _free_children(hblock_t * p)
{
	hlist_item_t * i, * tmp;
	
#ifndef NDEBUG
	



	assert(p && p->magic == HH_MAGIC);
	p->magic = 0; 
#endif
	hlist_for_each_safe(i, tmp, &p->children)
	{
		hblock_t * q = structof(i, hblock_t, siblings);
		_free_children(q);
		allocator(q, 0);
	}
}

