




































#ifndef	_CDEFS_H_
#define	_CDEFS_H_

#if defined(__cplusplus)
#define	__BEGIN_DECLS	extern "C" {
#define	__END_DECLS	}
#else
#define	__BEGIN_DECLS
#define	__END_DECLS
#endif








#if defined(__STDC__) || defined(__cplusplus) || defined(_WINDOWS) || defined(XP_OS2)
#define	__P(protos)	protos		/* full-blown ANSI C */
#define	__CONCAT(x,y)	x ## y
#define	__STRING(x)	#x


#ifndef __const
#define	__const		const		/* define reserved names to standard */
#endif  
#define	__signed	signed
#define	__volatile	volatile
#ifndef _WINDOWS
#if defined(__cplusplus)
#define	__inline	inline		/* convert to C++ keyword */
#else
#if !defined(__GNUC__) && !defined(__MWERKS__)
#define	__inline
#endif 
#endif 
#endif 

#else	
#define	__P(protos)	()		/* traditional C preprocessor */
#define	__CONCAT(x,y)	x/**/y
#define	__STRING(x)	"x"

#ifndef __GNUC__
#define	__const
#define	__inline
#define	__signed
#define	__volatile








#ifndef	NO_ANSI_KEYWORDS
#define	const
#define	inline
#define	signed
#define	volatile
#endif
#endif	
#endif	









#if !defined(__GNUC__) || __GNUC__ < 2 || __GNUC_MINOR__ < 5
#define	__attribute__(x)
#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
#define	__dead		__volatile
#define	__pure		__const
#endif
#endif


#ifndef __dead
#define	__dead
#define	__pure
#endif

#endif 
