#define	JEMALLOC_CHUNK_MMAP_C_
#include "jemalloc/internal/jemalloc_internal.h"




static void	*pages_map(void *addr, size_t size);
static void	pages_unmap(void *addr, size_t size);
static void	*chunk_alloc_mmap_slow(size_t size, size_t alignment,
    bool *zero);



static void *
pages_map(void *addr, size_t size)
{
	void *ret;

	assert(size != 0);

#ifdef _WIN32
	



	ret = VirtualAlloc(addr, size, MEM_COMMIT | MEM_RESERVE,
	    PAGE_READWRITE);
#else
	



	ret = mmap(addr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON,
	    -1, 0);
	assert(ret != NULL);

	if (ret == MAP_FAILED)
		ret = NULL;
	else if (addr != NULL && ret != addr) {
		


		if (munmap(ret, size) == -1) {
			char buf[BUFERROR_BUF];

			buferror(get_errno(), buf, sizeof(buf));
			malloc_printf("<jemalloc: Error in munmap(): %s\n",
			    buf);
			if (opt_abort)
				abort();
		}
		ret = NULL;
	}
#endif
	assert(ret == NULL || (addr == NULL && ret != addr)
	    || (addr != NULL && ret == addr));
	return (ret);
}

static void
pages_unmap(void *addr, size_t size)
{

#ifdef _WIN32
	if (VirtualFree(addr, 0, MEM_RELEASE) == 0)
#else
	if (munmap(addr, size) == -1)
#endif
	{
		char buf[BUFERROR_BUF];

		buferror(get_errno(), buf, sizeof(buf));
		malloc_printf("<jemalloc>: Error in "
#ifdef _WIN32
		              "VirtualFree"
#else
		              "munmap"
#endif
		              "(): %s\n", buf);
		if (opt_abort)
			abort();
	}
}

static void *
pages_trim(void *addr, size_t alloc_size, size_t leadsize, size_t size)
{
	void *ret = (void *)((uintptr_t)addr + leadsize);

	assert(alloc_size >= leadsize + size);
#ifdef _WIN32
	{
		void *new_addr;

		pages_unmap(addr, alloc_size);
		new_addr = pages_map(ret, size);
		if (new_addr == ret)
			return (ret);
		if (new_addr)
			pages_unmap(new_addr, size);
		return (NULL);
	}
#else
	{
		size_t trailsize = alloc_size - leadsize - size;

		if (leadsize != 0)
			pages_unmap(addr, leadsize);
		if (trailsize != 0)
			pages_unmap((void *)((uintptr_t)ret + size), trailsize);
		return (ret);
	}
#endif
}

bool
pages_purge(void *addr, size_t length)
{
	bool unzeroed;

#ifdef _WIN32
	VirtualAlloc(addr, length, MEM_RESET, PAGE_READWRITE);
	unzeroed = true;
#else
#  ifdef JEMALLOC_PURGE_MADVISE_DONTNEED
#    define JEMALLOC_MADV_PURGE MADV_DONTNEED
#    define JEMALLOC_MADV_ZEROS true
#  elif defined(JEMALLOC_PURGE_MADVISE_FREE)
#    define JEMALLOC_MADV_PURGE MADV_FREE
#    define JEMALLOC_MADV_ZEROS false
#  else
#    error "No method defined for purging unused dirty pages."
#  endif
	int err = madvise(addr, length, JEMALLOC_MADV_PURGE);
	unzeroed = (JEMALLOC_MADV_ZEROS == false || err != 0);
#  undef JEMALLOC_MADV_PURGE
#  undef JEMALLOC_MADV_ZEROS
#endif
	return (unzeroed);
}

static void *
chunk_alloc_mmap_slow(size_t size, size_t alignment, bool *zero)
{
	void *ret, *pages;
	size_t alloc_size, leadsize;

	alloc_size = size + alignment - PAGE;
	
	if (alloc_size < size)
		return (NULL);
	do {
		pages = pages_map(NULL, alloc_size);
		if (pages == NULL)
			return (NULL);
		leadsize = ALIGNMENT_CEILING((uintptr_t)pages, alignment) -
		    (uintptr_t)pages;
		ret = pages_trim(pages, alloc_size, leadsize, size);
	} while (ret == NULL);

	assert(ret != NULL);
	*zero = true;
	return (ret);
}

void *
chunk_alloc_mmap(size_t size, size_t alignment, bool *zero)
{
	void *ret;
	size_t offset;

	












	assert(alignment != 0);
	assert((alignment & chunksize_mask) == 0);

	ret = pages_map(NULL, size);
	if (ret == NULL)
		return (NULL);
	offset = ALIGNMENT_ADDR2OFFSET(ret, alignment);
	if (offset != 0) {
		pages_unmap(ret, size);
		return (chunk_alloc_mmap_slow(size, alignment, zero));
	}

	assert(ret != NULL);
	*zero = true;
	return (ret);
}

bool
chunk_dealloc_mmap(void *chunk, size_t size)
{

	if (config_munmap)
		pages_unmap(chunk, size);

	return (config_munmap == false);
}
