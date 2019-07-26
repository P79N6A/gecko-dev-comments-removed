



























#include <stdio.h>
#if	defined(HAVE_STDLIB)
#include <stdlib.h>
#include <string.h>
#endif	
#if	defined(SYS_UNIX)
#include "unix.h"
#endif	
#if	defined(SYS_OS9)
#include "os9.h"
#endif	

#if	!defined(SIZE_T)
#define SIZE_T	unsigned int
#endif	

typedef unsigned char	CHAR;

#if	defined(HIDE)
#define STATIC	static
#else
#define STATIC
#endif	

#if	!defined(CONST)
#if	defined(__STDC__)
#define CONST	const
#else
#define CONST
#endif	
#endif	


#define MEM_INC		64
#define SCREEN_INC	256

#define DISPOSE(p)	free((char *)(p))
#define NEW(T, c)	\
	((T *)malloc((unsigned int)(sizeof (T) * (c))))
#define RENEW(p, T, c)	\
	(p = (T *)realloc((char *)(p), (unsigned int)(sizeof (T) * (c))))
#define COPYFROMTO(new, p, len)	\
	(void)memcpy((char *)(new), (char *)(p), (int)(len))





extern unsigned rl_eof;
extern unsigned rl_erase;
extern unsigned rl_intr;
extern unsigned rl_kill;
extern unsigned rl_quit;
extern char	*rl_complete();
extern int	rl_list_possib();
extern void	rl_ttyset(int);
extern void	rl_add_slash();

#if	!defined(HAVE_STDLIB)
extern char	*getenv();
extern char	*malloc();
extern char	*realloc();
extern char	*memcpy();
extern char	*strcat();
extern char	*strchr();
extern char	*strrchr();
extern char	*strcpy();
extern char	*strdup();
extern int	strcmp();
extern int	strlen();
extern int	strncmp();
#endif	

