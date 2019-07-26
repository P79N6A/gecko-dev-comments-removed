












































#ifndef HMAC_H
#define HMAC_H

#include "auth.h"
#include "sha1.h"

typedef struct {
  uint8_t    opad[64];
  sha1_ctx_t ctx;
  sha1_ctx_t init_ctx;
} hmac_ctx_t;

err_status_t
hmac_alloc(auth_t **a, int key_len, int out_len);

err_status_t
hmac_dealloc(auth_t *a);

err_status_t
hmac_init(hmac_ctx_t *state, const uint8_t *key, int key_len);

err_status_t
hmac_start(hmac_ctx_t *state);

err_status_t
hmac_update(hmac_ctx_t *state, const uint8_t *message, int msg_octets);

err_status_t
hmac_compute(hmac_ctx_t *state, const void *message,
	     int msg_octets, int tag_len, uint8_t *result);


#endif 
