




























#ifndef MEMCLUSTER_H
#define MEMCLUSTER_H

#include <stdio.h>

#define meminit		__meminit
#ifdef MEMCLUSTER_DEBUG
#define memget(s)	__memget_debug(s, __FILE__, __LINE__)
#define memput(p, s)	__memput_debug(p, s, __FILE__, __LINE__)
#else 
#ifdef MEMCLUSTER_RECORD
#define memget(s)	__memget_record(s, __FILE__, __LINE__)
#define memput(p, s)	__memput_record(p, s, __FILE__, __LINE__)
#else 
#define memget		__memget
#define memput		__memput
#endif 
#endif 
#define memstats	__memstats
#define memactive	__memactive

int	meminit(size_t, size_t);
void *	__memget(size_t);
void 	__memput(void *, size_t);
void *	__memget_debug(size_t, const char *, int);
void 	__memput_debug(void *, size_t, const char *, int);
void *	__memget_record(size_t, const char *, int);
void 	__memput_record(void *, size_t, const char *, int);
void 	memstats(FILE *);
int	memactive(void);

#endif 
