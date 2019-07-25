
#ifdef JEMALLOC_H_TYPES

#endif 

#ifdef JEMALLOC_H_STRUCTS

#endif 

#ifdef JEMALLOC_H_EXTERNS

#endif 

#ifdef JEMALLOC_H_INLINES

#ifndef JEMALLOC_ENABLE_INLINE
void	mb_write(void);
#endif

#if (defined(JEMALLOC_ENABLE_INLINE) || defined(JEMALLOC_MB_C_))
#ifdef __i386__









JEMALLOC_INLINE void
mb_write(void)
{

#  if 0
	
	asm volatile ("pusha;"
	    "xor  %%eax,%%eax;"
	    "cpuid;"
	    "popa;"
	    : 
	    : 
	    : "memory" 
	    );
#else
	



	asm volatile ("nop;"
	    : 
	    : 
	    : "memory" 
	    );
#endif
}
#elif (defined(__amd64__) || defined(__x86_64__))
JEMALLOC_INLINE void
mb_write(void)
{

	asm volatile ("sfence"
	    : 
	    : 
	    : "memory" 
	    );
}
#elif defined(__powerpc__)
JEMALLOC_INLINE void
mb_write(void)
{

	asm volatile ("eieio"
	    : 
	    : 
	    : "memory" 
	    );
}
#elif defined(__sparc64__)
JEMALLOC_INLINE void
mb_write(void)
{

	asm volatile ("membar #StoreStore"
	    : 
	    : 
	    : "memory" 
	    );
}
#elif defined(__tile__)
JEMALLOC_INLINE void
mb_write(void)
{

	__sync_synchronize();
}
#else




JEMALLOC_INLINE void
mb_write(void)
{
	malloc_mutex_t mtx;

	malloc_mutex_init(&mtx);
	malloc_mutex_lock(&mtx);
	malloc_mutex_unlock(&mtx);
}
#endif
#endif

#endif 

