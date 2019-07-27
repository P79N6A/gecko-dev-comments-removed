








































#ifndef __SB_STRFUNCS_H__
#define __SB_STRFUNCS_H__

#include <stdarg.h>


#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/prim_type.h>


#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif





SPHINXBASE_EXPORT
char *string_join(const char *base, ...);




enum string_edge_e {
    STRING_START,	
    STRING_END,		
    STRING_BOTH		
};







SPHINXBASE_EXPORT
char *string_trim(char *string, enum string_edge_e which);








SPHINXBASE_EXPORT
double atof_c(char const *str);











SPHINXBASE_EXPORT
int32 str2words (char *line,	



		 char **wptr,	











		 int32 n_wptr	

	);














SPHINXBASE_EXPORT
int32 nextword (char *line, 

		const char *delim, 


		char **word,


		char *delimfound 


	);

#ifdef __cplusplus
}
#endif


#endif
