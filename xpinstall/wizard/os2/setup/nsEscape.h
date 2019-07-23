








































#ifndef _ESCAPE_H_
#define _ESCAPE_H_

#define NS_COM
#define PRUnichar char


typedef enum {
	url_XAlphas		= (1<<0)
,	url_XPAlphas	= (1<<1)
,	url_Path		= (1<<2)
} nsEscapeMask;

#ifdef __cplusplus
extern "C" {
#endif
NS_COM char * nsEscape(const char * str, nsEscapeMask mask);
	

NS_COM char * nsUnescape(char * str);
	



NS_COM char * nsEscapeCount(const char * str, PRInt32 len, nsEscapeMask mask, PRInt32* out_len);
	



NS_COM PRInt32 nsUnescapeCount (char * str);
	




#ifdef __cplusplus
}
#endif

#endif

