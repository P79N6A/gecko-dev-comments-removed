#define	JEMALLOC_CHUNK_DSS_C_
#include "jemalloc/internal/jemalloc_internal.h"







static malloc_mutex_t	dss_mtx;


static void		*dss_base;

static void		*dss_prev;

static void		*dss_max;



#ifndef JEMALLOC_HAVE_SBRK
static void *
sbrk(intptr_t increment)
{

	not_implemented();

	return (NULL);
}
#endif

void *
chunk_alloc_dss(size_t size, size_t alignment, bool *zero)
{
	void *ret;

	cassert(config_dss);
	assert(size > 0 && (size & chunksize_mask) == 0);
	assert(alignment > 0 && (alignment & chunksize_mask) == 0);

	



	if ((intptr_t)size < 0)
		return (NULL);

	malloc_mutex_lock(&dss_mtx);
	if (dss_prev != (void *)-1) {
		size_t gap_size, cpad_size;
		void *cpad, *dss_next;
		intptr_t incr;

		




		do {
			
			dss_max = sbrk(0);
			



			gap_size = (chunksize - CHUNK_ADDR2OFFSET(dss_max)) &
			    chunksize_mask;
			




			cpad = (void *)((uintptr_t)dss_max + gap_size);
			ret = (void *)ALIGNMENT_CEILING((uintptr_t)dss_max,
			    alignment);
			cpad_size = (uintptr_t)ret - (uintptr_t)cpad;
			dss_next = (void *)((uintptr_t)ret + size);
			if ((uintptr_t)ret < (uintptr_t)dss_max ||
			    (uintptr_t)dss_next < (uintptr_t)dss_max) {
				
				malloc_mutex_unlock(&dss_mtx);
				return (NULL);
			}
			incr = gap_size + cpad_size + size;
			dss_prev = sbrk(incr);
			if (dss_prev == dss_max) {
				
				dss_max = dss_next;
				malloc_mutex_unlock(&dss_mtx);
				if (cpad_size != 0)
					chunk_dealloc(cpad, cpad_size, true);
				if (*zero) {
					VALGRIND_MAKE_MEM_UNDEFINED(ret, size);
					memset(ret, 0, size);
				}
				return (ret);
			}
		} while (dss_prev != (void *)-1);
	}
	malloc_mutex_unlock(&dss_mtx);

	return (NULL);
}

bool
chunk_in_dss(void *chunk)
{
	bool ret;

	cassert(config_dss);

	malloc_mutex_lock(&dss_mtx);
	if ((uintptr_t)chunk >= (uintptr_t)dss_base
	    && (uintptr_t)chunk < (uintptr_t)dss_max)
		ret = true;
	else
		ret = false;
	malloc_mutex_unlock(&dss_mtx);

	return (ret);
}

bool
chunk_dss_boot(void)
{

	cassert(config_dss);

	if (malloc_mutex_init(&dss_mtx))
		return (true);
	dss_base = sbrk(0);
	dss_prev = dss_base;
	dss_max = dss_base;

	return (false);
}

void
chunk_dss_prefork(void)
{

	if (config_dss)
		malloc_mutex_prefork(&dss_mtx);
}

void
chunk_dss_postfork_parent(void)
{

	if (config_dss)
		malloc_mutex_postfork_parent(&dss_mtx);
}

void
chunk_dss_postfork_child(void)
{

	if (config_dss)
		malloc_mutex_postfork_child(&dss_mtx);
}


