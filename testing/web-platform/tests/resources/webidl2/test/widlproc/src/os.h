












#ifndef os_h
#define os_h


#if defined(__gnu_linux__)



#elif defined(_MSC_VER)

#define strncasecmp strnicmp
#if _MSC_VER < 1900
#define snprintf _snprintf
#endif
#define va_copy(a,b) ((a)=(b))

#endif

#endif 
