





































#ifndef _plstr_h
#define _plstr_h





















#include "prtypes.h"

PR_BEGIN_EXTERN_C






PR_EXTERN(PRUint32)
PL_strlen(const char *str);









PR_EXTERN(PRUint32)
PL_strnlen(const char *str, PRUint32 max);









PR_EXTERN(char *)
PL_strcpy(char *dest, const char *src);











PR_EXTERN(char *)
PL_strncpy(char *dest, const char *src, PRUint32 max);





















PR_EXTERN(char *)
PL_strncpyz(char *dest, const char *src, PRUint32 max);












PR_EXTERN(char *)
PL_strdup(const char *s);







PR_EXTERN(void)
PL_strfree(char *s);













PR_EXTERN(char *)
PL_strndup(const char *s, PRUint32 max);










PR_EXTERN(char *)
PL_strcat(char *dst, const char *src);













PR_EXTERN(char *)
PL_strncat(char *dst, const char *src, PRUint32 max);












PR_EXTERN(char *)
PL_strcatn(char *dst, PRUint32 max, const char *src);










PR_EXTERN(PRIntn)
PL_strcmp(const char *a, const char *b);












PR_EXTERN(PRIntn)
PL_strncmp(const char *a, const char *b, PRUint32 max);










PR_EXTERN(PRIntn)
PL_strcasecmp(const char *a, const char *b);










PR_EXTERN(PRIntn)
PL_strncasecmp(const char *a, const char *b, PRUint32 max);









PR_EXTERN(char *)
PL_strchr(const char *s, char c);









PR_EXTERN(char *)
PL_strrchr(const char *s, char c);










PR_EXTERN(char *)
PL_strnchr(const char *s, char c, PRUint32 n);










PR_EXTERN(char *)
PL_strnrchr(const char *s, char c, PRUint32 n);














PR_EXTERN(char *)
PL_strpbrk(const char *s, const char *list);









PR_EXTERN(char *)
PL_strprbrk(const char *s, const char *list);









PR_EXTERN(char *)
PL_strnpbrk(const char *s, const char *list, PRUint32 n);









PR_EXTERN(char *)
PL_strnprbrk(const char *s, const char *list, PRUint32 n);








PR_EXTERN(char *)
PL_strstr(const char *big, const char *little);








PR_EXTERN(char *)
PL_strrstr(const char *big, const char *little);









PR_EXTERN(char *)
PL_strnstr(const char *big, const char *little, PRUint32 n);









PR_EXTERN(char *)
PL_strnrstr(const char *big, const char *little, PRUint32 max);








PR_EXTERN(char *)
PL_strcasestr(const char *big, const char *little);








PR_EXTERN(char *)
PL_strcaserstr(const char *big, const char *little);









PR_EXTERN(char *)
PL_strncasestr(const char *big, const char *little, PRUint32 max);









PR_EXTERN(char *)
PL_strncaserstr(const char *big, const char *little, PRUint32 max);




















PR_EXTERN(char *)
PL_strtok_r(char *s1, const char *s2, char **lasts);







PR_END_EXTERN_C

#endif 
