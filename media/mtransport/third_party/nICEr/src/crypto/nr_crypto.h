

































#ifndef _nr_crypto_h
#define _nr_crypto_h


typedef struct nr_ice_crypto_vtbl_ {
  int (*random_bytes)(UCHAR *buf, int len);
  int (*hmac_sha1)(UCHAR *key, int key_l, UCHAR *buf, int buf_l, UCHAR digest[20]);
} nr_ice_crypto_vtbl;

extern nr_ice_crypto_vtbl *nr_crypto_vtbl;

#define nr_crypto_random_bytes(a,b) nr_crypto_vtbl->random_bytes(a,b)
#define nr_crypto_hmac_sha1(a,b,c,d,e) nr_crypto_vtbl->hmac_sha1(a,b,c,d,e)

#endif

