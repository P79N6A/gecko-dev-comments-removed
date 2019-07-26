
#ifdef JEMALLOC_H_TYPES

























#define	prng32(r, lg_range, state, a, c) do {				\
	assert(lg_range > 0);						\
	assert(lg_range <= 32);						\
									\
	r = (state * (a)) + (c);					\
	state = r;							\
	r >>= (32 - lg_range);						\
} while (false)


#define	prng64(r, lg_range, state, a, c) do {				\
	assert(lg_range > 0);						\
	assert(lg_range <= 64);						\
									\
	r = (state * (a)) + (c);					\
	state = r;							\
	r >>= (64 - lg_range);						\
} while (false)

#endif 

#ifdef JEMALLOC_H_STRUCTS

#endif 

#ifdef JEMALLOC_H_EXTERNS

#endif 

#ifdef JEMALLOC_H_INLINES

#endif 

