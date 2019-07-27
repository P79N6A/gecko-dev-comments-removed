





























#ifndef _OAES_LIB_H
#define _OAES_LIB_H

#include "oaes_common.h"

#ifdef __cplusplus 
extern "C" {
#endif

#ifdef _WIN32
#	ifdef OAES_SHARED
#		ifdef oaes_lib_EXPORTS
#			define OAES_API __declspec(dllexport)
#		else
#			define OAES_API __declspec(dllimport)
#		endif
#	else
#		define OAES_API
#	endif
#else
#	define OAES_API
#endif 

#define OAES_BLOCK_SIZE 16

typedef void OAES_CTX;






#define OAES_OPTION_NONE 0

#define OAES_OPTION_ECB 1



#define OAES_OPTION_CBC 2

#ifdef OAES_DEBUG
typedef int ( * oaes_step_cb ) (
		const uint8_t state[OAES_BLOCK_SIZE],
		const char * step_name,
		int step_count,
		void * user_data );


#define OAES_OPTION_STEP_ON 4

#define OAES_OPTION_STEP_OFF 8
#endif 

typedef uint16_t OAES_OPTION;




































OAES_API OAES_CTX * oaes_alloc();

OAES_API OAES_RET oaes_free( OAES_CTX ** ctx );

OAES_API OAES_RET oaes_set_option( OAES_CTX * ctx,
		OAES_OPTION option, const void * value );

OAES_API OAES_RET oaes_key_gen_128( OAES_CTX * ctx );

OAES_API OAES_RET oaes_key_gen_192( OAES_CTX * ctx );

OAES_API OAES_RET oaes_key_gen_256( OAES_CTX * ctx );



OAES_API OAES_RET oaes_key_export( OAES_CTX * ctx,
		uint8_t * data, size_t * data_len );



OAES_API OAES_RET oaes_key_export_data( OAES_CTX * ctx,
		uint8_t * data, size_t * data_len );


OAES_API OAES_RET oaes_key_import( OAES_CTX * ctx,
		const uint8_t * data, size_t data_len );


OAES_API OAES_RET oaes_key_import_data( OAES_CTX * ctx,
		const uint8_t * data, size_t data_len );


OAES_API OAES_RET oaes_encrypt( OAES_CTX * ctx,
		const uint8_t * m, size_t m_len, uint8_t * c, size_t * c_len );


OAES_API OAES_RET oaes_decrypt( OAES_CTX * ctx,
		const uint8_t * c, size_t c_len, uint8_t * m, size_t * m_len );


OAES_API OAES_RET oaes_sprintf(
		char * buf, size_t * buf_len, const uint8_t * data, size_t data_len );

#ifdef __cplusplus 
}
#endif

#endif
