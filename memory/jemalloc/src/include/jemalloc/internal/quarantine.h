
#ifdef JEMALLOC_H_TYPES


#define	JEMALLOC_VALGRIND_QUARANTINE_DEFAULT	(ZU(1) << 24)

#endif 

#ifdef JEMALLOC_H_STRUCTS

#endif 

#ifdef JEMALLOC_H_EXTERNS

void	quarantine(void *ptr);
bool	quarantine_boot(void);

#endif 

#ifdef JEMALLOC_H_INLINES

#endif 


