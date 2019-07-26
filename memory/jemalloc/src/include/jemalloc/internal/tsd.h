
#ifdef JEMALLOC_H_TYPES


#define	MALLOC_TSD_CLEANUPS_MAX	8

typedef bool (*malloc_tsd_cleanup_t)(void);

#if (!defined(JEMALLOC_MALLOC_THREAD_CLEANUP) && !defined(JEMALLOC_TLS) && \
    !defined(_WIN32))
typedef struct tsd_init_block_s tsd_init_block_t;
typedef struct tsd_init_head_s tsd_init_head_t;
#endif


















































#define	malloc_tsd_protos(a_attr, a_name, a_type)			\
a_attr bool								\
a_name##_tsd_boot(void);						\
a_attr a_type *								\
a_name##_tsd_get(void);							\
a_attr void								\
a_name##_tsd_set(a_type *val);


#ifdef JEMALLOC_MALLOC_THREAD_CLEANUP
#define	malloc_tsd_externs(a_name, a_type)				\
extern __thread a_type	a_name##_tls;					\
extern __thread bool	a_name##_initialized;				\
extern bool		a_name##_booted;
#elif (defined(JEMALLOC_TLS))
#define	malloc_tsd_externs(a_name, a_type)				\
extern __thread a_type	a_name##_tls;					\
extern pthread_key_t	a_name##_tsd;					\
extern bool		a_name##_booted;
#elif (defined(_WIN32))
#define	malloc_tsd_externs(a_name, a_type)				\
extern DWORD		a_name##_tsd;					\
extern bool		a_name##_booted;
#else
#define	malloc_tsd_externs(a_name, a_type)				\
extern pthread_key_t	a_name##_tsd;					\
extern tsd_init_head_t	a_name##_tsd_init_head;				\
extern bool		a_name##_booted;
#endif


#ifdef JEMALLOC_MALLOC_THREAD_CLEANUP
#define	malloc_tsd_data(a_attr, a_name, a_type, a_initializer)		\
a_attr __thread a_type JEMALLOC_TLS_MODEL				\
    a_name##_tls = a_initializer;					\
a_attr __thread bool JEMALLOC_TLS_MODEL					\
    a_name##_initialized = false;					\
a_attr bool		a_name##_booted = false;
#elif (defined(JEMALLOC_TLS))
#define	malloc_tsd_data(a_attr, a_name, a_type, a_initializer)		\
a_attr __thread a_type JEMALLOC_TLS_MODEL				\
    a_name##_tls = a_initializer;					\
a_attr pthread_key_t	a_name##_tsd;					\
a_attr bool		a_name##_booted = false;
#elif (defined(_WIN32))
#define	malloc_tsd_data(a_attr, a_name, a_type, a_initializer)		\
a_attr DWORD		a_name##_tsd;					\
a_attr bool		a_name##_booted = false;
#else
#define	malloc_tsd_data(a_attr, a_name, a_type, a_initializer)		\
a_attr pthread_key_t	a_name##_tsd;					\
a_attr tsd_init_head_t	a_name##_tsd_init_head = {			\
	ql_head_initializer(blocks),					\
	MALLOC_MUTEX_INITIALIZER					\
};									\
a_attr bool		a_name##_booted = false;
#endif


#ifdef JEMALLOC_MALLOC_THREAD_CLEANUP
#define	malloc_tsd_funcs(a_attr, a_name, a_type, a_initializer,		\
    a_cleanup)								\
/* Initialization/cleanup. */						\
a_attr bool								\
a_name##_tsd_cleanup_wrapper(void)					\
{									\
									\
	if (a_name##_initialized) {					\
		a_name##_initialized = false;				\
		a_cleanup(&a_name##_tls);				\
	}								\
	return (a_name##_initialized);					\
}									\
a_attr bool								\
a_name##_tsd_boot(void)							\
{									\
									\
	if (a_cleanup != malloc_tsd_no_cleanup) {			\
		malloc_tsd_cleanup_register(				\
		    &a_name##_tsd_cleanup_wrapper);			\
	}								\
	a_name##_booted = true;						\
	return (false);							\
}									\
/* Get/set. */								\
a_attr a_type *								\
a_name##_tsd_get(void)							\
{									\
									\
	assert(a_name##_booted);					\
	return (&a_name##_tls);						\
}									\
a_attr void								\
a_name##_tsd_set(a_type *val)						\
{									\
									\
	assert(a_name##_booted);					\
	a_name##_tls = (*val);						\
	if (a_cleanup != malloc_tsd_no_cleanup)				\
		a_name##_initialized = true;				\
}
#elif (defined(JEMALLOC_TLS))
#define	malloc_tsd_funcs(a_attr, a_name, a_type, a_initializer,		\
    a_cleanup)								\
/* Initialization/cleanup. */						\
a_attr bool								\
a_name##_tsd_boot(void)							\
{									\
									\
	if (a_cleanup != malloc_tsd_no_cleanup) {			\
		if (pthread_key_create(&a_name##_tsd, a_cleanup) != 0)	\
			return (true);					\
	}								\
	a_name##_booted = true;						\
	return (false);							\
}									\
/* Get/set. */								\
a_attr a_type *								\
a_name##_tsd_get(void)							\
{									\
									\
	assert(a_name##_booted);					\
	return (&a_name##_tls);						\
}									\
a_attr void								\
a_name##_tsd_set(a_type *val)						\
{									\
									\
	assert(a_name##_booted);					\
	a_name##_tls = (*val);						\
	if (a_cleanup != malloc_tsd_no_cleanup) {			\
		if (pthread_setspecific(a_name##_tsd,			\
		    (void *)(&a_name##_tls))) {				\
			malloc_write("<jemalloc>: Error"		\
			    " setting TSD for "#a_name"\n");		\
			if (opt_abort)					\
				abort();				\
		}							\
	}								\
}
#elif (defined(_WIN32))
#define	malloc_tsd_funcs(a_attr, a_name, a_type, a_initializer,		\
    a_cleanup)								\
/* Data structure. */							\
typedef struct {							\
	bool	initialized;						\
	a_type	val;							\
} a_name##_tsd_wrapper_t;						\
/* Initialization/cleanup. */						\
a_attr bool								\
a_name##_tsd_cleanup_wrapper(void)					\
{									\
	a_name##_tsd_wrapper_t *wrapper;				\
									\
	wrapper = (a_name##_tsd_wrapper_t *) TlsGetValue(a_name##_tsd);	\
	if (wrapper == NULL)						\
		return (false);						\
	if (a_cleanup != malloc_tsd_no_cleanup &&			\
	    wrapper->initialized) {					\
		a_type val = wrapper->val;				\
		a_type tsd_static_data = a_initializer;			\
		wrapper->initialized = false;				\
		wrapper->val = tsd_static_data;				\
		a_cleanup(&val);					\
		if (wrapper->initialized) {				\
			/* Trigger another cleanup round. */		\
			return (true);					\
		}							\
	}								\
	malloc_tsd_dalloc(wrapper);					\
	return (false);							\
}									\
a_attr bool								\
a_name##_tsd_boot(void)							\
{									\
									\
	a_name##_tsd = TlsAlloc();					\
	if (a_name##_tsd == TLS_OUT_OF_INDEXES)				\
		return (true);						\
	if (a_cleanup != malloc_tsd_no_cleanup) {			\
		malloc_tsd_cleanup_register(				\
		    &a_name##_tsd_cleanup_wrapper);			\
	}								\
	a_name##_booted = true;						\
	return (false);							\
}									\
/* Get/set. */								\
a_attr a_name##_tsd_wrapper_t *						\
a_name##_tsd_get_wrapper(void)						\
{									\
	a_name##_tsd_wrapper_t *wrapper = (a_name##_tsd_wrapper_t *)	\
	    TlsGetValue(a_name##_tsd);					\
									\
	if (wrapper == NULL) {						\
		wrapper = (a_name##_tsd_wrapper_t *)			\
		    malloc_tsd_malloc(sizeof(a_name##_tsd_wrapper_t));	\
		if (wrapper == NULL) {					\
			malloc_write("<jemalloc>: Error allocating"	\
			    " TSD for "#a_name"\n");			\
			abort();					\
		} else {						\
			static a_type tsd_static_data = a_initializer;	\
			wrapper->initialized = false;			\
			wrapper->val = tsd_static_data;			\
		}							\
		if (!TlsSetValue(a_name##_tsd, (void *)wrapper)) {	\
			malloc_write("<jemalloc>: Error setting"	\
			    " TSD for "#a_name"\n");			\
			abort();					\
		}							\
	}								\
	return (wrapper);						\
}									\
a_attr a_type *								\
a_name##_tsd_get(void)							\
{									\
	a_name##_tsd_wrapper_t *wrapper;				\
									\
	assert(a_name##_booted);					\
	wrapper = a_name##_tsd_get_wrapper();				\
	return (&wrapper->val);						\
}									\
a_attr void								\
a_name##_tsd_set(a_type *val)						\
{									\
	a_name##_tsd_wrapper_t *wrapper;				\
									\
	assert(a_name##_booted);					\
	wrapper = a_name##_tsd_get_wrapper();				\
	wrapper->val = *(val);						\
	if (a_cleanup != malloc_tsd_no_cleanup)				\
		wrapper->initialized = true;				\
}
#else
#define	malloc_tsd_funcs(a_attr, a_name, a_type, a_initializer,		\
    a_cleanup)								\
/* Data structure. */							\
typedef struct {							\
	bool	initialized;						\
	a_type	val;							\
} a_name##_tsd_wrapper_t;						\
/* Initialization/cleanup. */						\
a_attr void								\
a_name##_tsd_cleanup_wrapper(void *arg)					\
{									\
	a_name##_tsd_wrapper_t *wrapper = (a_name##_tsd_wrapper_t *)arg;\
									\
	if (a_cleanup != malloc_tsd_no_cleanup &&			\
	    wrapper->initialized) {					\
		wrapper->initialized = false;				\
		a_cleanup(&wrapper->val);				\
		if (wrapper->initialized) {				\
			/* Trigger another cleanup round. */		\
			if (pthread_setspecific(a_name##_tsd,		\
			    (void *)wrapper)) {				\
				malloc_write("<jemalloc>: Error"	\
				    " setting TSD for "#a_name"\n");	\
				if (opt_abort)				\
					abort();			\
			}						\
			return;						\
		}							\
	}								\
	malloc_tsd_dalloc(wrapper);					\
}									\
a_attr bool								\
a_name##_tsd_boot(void)							\
{									\
									\
	if (pthread_key_create(&a_name##_tsd,				\
	    a_name##_tsd_cleanup_wrapper) != 0)				\
		return (true);						\
	a_name##_booted = true;						\
	return (false);							\
}									\
/* Get/set. */								\
a_attr a_name##_tsd_wrapper_t *						\
a_name##_tsd_get_wrapper(void)						\
{									\
	a_name##_tsd_wrapper_t *wrapper = (a_name##_tsd_wrapper_t *)	\
	    pthread_getspecific(a_name##_tsd);				\
									\
	if (wrapper == NULL) {						\
		tsd_init_block_t block;					\
		wrapper = tsd_init_check_recursion(			\
		    &a_name##_tsd_init_head, &block);			\
		if (wrapper)						\
		    return (wrapper);					\
		wrapper = (a_name##_tsd_wrapper_t *)			\
		    malloc_tsd_malloc(sizeof(a_name##_tsd_wrapper_t));	\
		block.data = wrapper;					\
		if (wrapper == NULL) {					\
			malloc_write("<jemalloc>: Error allocating"	\
			    " TSD for "#a_name"\n");			\
			abort();					\
		} else {						\
			static a_type tsd_static_data = a_initializer;	\
			wrapper->initialized = false;			\
			wrapper->val = tsd_static_data;			\
		}							\
		if (pthread_setspecific(a_name##_tsd,			\
		    (void *)wrapper)) {					\
			malloc_write("<jemalloc>: Error setting"	\
			    " TSD for "#a_name"\n");			\
			abort();					\
		}							\
		tsd_init_finish(&a_name##_tsd_init_head, &block);	\
	}								\
	return (wrapper);						\
}									\
a_attr a_type *								\
a_name##_tsd_get(void)							\
{									\
	a_name##_tsd_wrapper_t *wrapper;				\
									\
	assert(a_name##_booted);					\
	wrapper = a_name##_tsd_get_wrapper();				\
	return (&wrapper->val);						\
}									\
a_attr void								\
a_name##_tsd_set(a_type *val)						\
{									\
	a_name##_tsd_wrapper_t *wrapper;				\
									\
	assert(a_name##_booted);					\
	wrapper = a_name##_tsd_get_wrapper();				\
	wrapper->val = *(val);						\
	if (a_cleanup != malloc_tsd_no_cleanup)				\
		wrapper->initialized = true;				\
}
#endif

#endif 

#ifdef JEMALLOC_H_STRUCTS

#if (!defined(JEMALLOC_MALLOC_THREAD_CLEANUP) && !defined(JEMALLOC_TLS) && \
    !defined(_WIN32))
struct tsd_init_block_s {
	ql_elm(tsd_init_block_t)	link;
	pthread_t			thread;
	void				*data;
};
struct tsd_init_head_s {
	ql_head(tsd_init_block_t)	blocks;
	malloc_mutex_t			lock;
};
#endif

#endif 

#ifdef JEMALLOC_H_EXTERNS

void	*malloc_tsd_malloc(size_t size);
void	malloc_tsd_dalloc(void *wrapper);
void	malloc_tsd_no_cleanup(void *);
void	malloc_tsd_cleanup_register(bool (*f)(void));
void	malloc_tsd_boot(void);
#if (!defined(JEMALLOC_MALLOC_THREAD_CLEANUP) && !defined(JEMALLOC_TLS) && \
    !defined(_WIN32))
void	*tsd_init_check_recursion(tsd_init_head_t *head,
    tsd_init_block_t *block);
void	tsd_init_finish(tsd_init_head_t *head, tsd_init_block_t *block);
#endif

#endif 

#ifdef JEMALLOC_H_INLINES

#endif 

