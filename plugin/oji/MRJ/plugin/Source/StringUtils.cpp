







































#include "StringUtils.h"

#include <string.h>

inline unsigned char toupper(unsigned char c)
{
	return (c >= 'a' && c <= 'z') ? (c - ('a' - 'A')) : c;
}

int strcasecmp(const char * str1, const char * str2)
{
#if !__POWERPC__
	
	const	unsigned char * p1 = (unsigned char *) str1;
	const	unsigned char * p2 = (unsigned char *) str2;
				unsigned char		c1, c2;
	
	while (toupper(c1 = *p1++) == toupper(c2 = *p2++))
		if (!c1)
			return(0);

#else
	
	const	unsigned char * p1 = (unsigned char *) str1 - 1;
	const	unsigned char * p2 = (unsigned char *) str2 - 1;
				unsigned long		c1, c2;
		
	while (toupper(c1 = *++p1) == toupper(c2 = *++p2))
		if (!c1)
			return(0);

#endif
	
	return(toupper(c1) - toupper(c2));
}

char* strdup(const char* str)
{
	if (str != NULL) {
		char* result = new char[::strlen(str) + 1];
		if (result != NULL)
			::strcpy(result, str);
		return result;
	}
	return NULL;
}
