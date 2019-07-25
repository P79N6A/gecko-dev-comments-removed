





















#ifndef _DICTIONARY_H_
#define _DICTIONARY_H_





#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
















typedef struct _dictionary_ {
	int				n ;		
	int				size ;	
	char 		**	val ;	
	char 		**  key ;	
	unsigned	 *	hash ;	
} dictionary ;


















unsigned dictionary_hash(char * key);












dictionary * dictionary_new(int size);










void dictionary_del(dictionary * vd);















char * dictionary_get(dictionary * d, char * key, char * def);




























int dictionary_set(dictionary * vd, char * key, char * val);












void dictionary_unset(dictionary * d, char * key);














void dictionary_dump(dictionary * d, FILE * out);

#endif
