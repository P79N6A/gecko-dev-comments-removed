#define	JEMALLOC_CHUNK_DSS_C_
#include "jemalloc/internal/jemalloc_internal.h"



const char	*dss_prec_names[] = {
	"disabled",
	"primary",
	"secondary",
	"N/A"
};


static dss_prec_t	dss_prec_default = DSS_PREC_DEFAULT;





static malloc_mutex_t	dss_mtx;


static void		*dss_base;

static void		*dss_prev;

static void		*dss_max;



static void *
chunk_dss_sbrk(intptr_t increment)
{

#ifdef JEMALLOC_HAVE_SBRK
	return (sbrk(increment));
#else
	not_implemented();
	return (NULL);
#endif
}

dss_prec_t
chunk_dss_prec_get(void)
{
	dss_prec_t ret;

	if (config_dss == false)
		return (dss_prec_disabled);
	malloc_mutex_lock(&dss_mtx);
	ret = dss_prec_default;
	malloc_mutex_unlock(&dss_mtx);
	return (ret);
}

bool
chunk_dss_prec_set(dss_prec_t dss_prec)
{

	if (config_dss == false)
		return (true);
	malloc_mutex_lock(&dss_mtx);
	dss_prec_default = dss_prec;
	malloc_mutex_unlock(&dss_mtx);
	return (false);
}

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
			
			dss_max = chunk_dss_sbrk(0);
			



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
			dss_prev = chunk_dss_sbrk(incr);
			if (dss_prev == dss_max) {
				
				dss_max = dss_next;
				malloc_mutex_unlock(&dss_mtx);
				if (cpad_size != 0)
					chunk_unmap(cpad, cpad_size);
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
	dss_base = chunk_dss_sbrk(0);
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


