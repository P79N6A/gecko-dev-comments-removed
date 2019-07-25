









































#ifndef nsIDNKitWrapper_h__
#define nsIDNKitWrapper_h__

#ifdef __cplusplus
extern "C" {
#endif

#include "prtypes.h"




typedef enum {
	idn_success,
	idn_notfound,
	idn_invalid_encoding,
	idn_invalid_syntax,
	idn_invalid_name,
	idn_invalid_message,
	idn_invalid_action,
	idn_invalid_codepoint,
	idn_invalid_length,
	idn_buffer_overflow,
	idn_noentry,
	idn_nomemory,
	idn_nofile,
	idn_nomapping,
	idn_context_required,
	idn_prohibited,
	idn_failure	
} idn_result_t;



      
typedef enum {
	idn_biditype_r_al,
	idn_biditype_l,
	idn_biditype_others
} idn_biditype_t;




typedef struct idn_nameprep *idn_nameprep_t;





#define IDN_NAMEPREP_CURRENT	"nameprep-11"

#undef assert
#define assert(a)
#define TRACE(a)



idn_result_t	race_decode_decompress(const char *from,
					       uint16_t *buf,
					       size_t buflen);
idn_result_t	race_compress_encode(const uint16_t *p,
					     int compress_mode,
					     char *to, size_t tolen);
int		get_compress_mode(uint16_t *p);
















idn_result_t
idn_nameprep_create(const char *version, idn_nameprep_t *handlep);




void
idn_nameprep_destroy(idn_nameprep_t handle);









idn_result_t
idn_nameprep_map(idn_nameprep_t handle, const uint32_t *from,
		 uint32_t *to, size_t tolen);












idn_result_t
idn_nameprep_isprohibited(idn_nameprep_t handle, const uint32_t *str,
			  const uint32_t **found);












idn_result_t
idn_nameprep_isunassigned(idn_nameprep_t handle, const uint32_t *str,
			  const uint32_t **found);











idn_result_t
idn_nameprep_isvalidbidi(idn_nameprep_t handle, const uint32_t *str,
			 const uint32_t **found);



#ifdef __cplusplus
}
#endif 

#endif
