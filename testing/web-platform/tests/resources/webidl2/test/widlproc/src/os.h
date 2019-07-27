












#ifndef os_h
#define os_h


#if defined(__gnu_linux__)



#elif defined(_MSC_VER)

#define inline __inline
#define strncasecmp strnicmp
#define snprintf _snprintf
#define va_copy(a,b) ((a)=(b))

#endif

#endif 
