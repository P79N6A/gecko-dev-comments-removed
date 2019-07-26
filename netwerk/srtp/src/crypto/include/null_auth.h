











































#ifndef NULL_AUTH_H
#define NULL_AUTH_H

#include "auth.h"

typedef struct {
	char foo;
} null_auth_ctx_t;

err_status_t
null_auth_alloc(auth_t **a, int key_len, int out_len);

err_status_t
null_auth_dealloc(auth_t *a);

err_status_t
null_auth_init(null_auth_ctx_t *state, const uint8_t *key, int key_len);

err_status_t
null_auth_compute (null_auth_ctx_t *state, uint8_t *message,
		   int msg_octets, int tag_len, uint8_t *result);


#endif 
