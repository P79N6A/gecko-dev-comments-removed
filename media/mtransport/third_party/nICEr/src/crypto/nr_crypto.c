

































static char *RCSSTRING __UNUSED__="$Id: nr_crypto.c,v 1.2 2008/04/28 17:59:01 ekr Exp $";

#include <nr_api.h>
#include "nr_crypto.h"

static int nr_ice_crypto_dummy_random_bytes(UCHAR *buf, int len)
  {
    fprintf(stderr,"Need to define crypto API implementation\n");

    exit(1);
  }

static int nr_ice_crypto_dummy_hmac_sha1(UCHAR *key, int key_l, UCHAR *buf, int buf_l, UCHAR digest[20])
  {
    fprintf(stderr,"Need to define crypto API implementation\n");

    exit(1);
  }

static nr_ice_crypto_vtbl nr_ice_crypto_dummy_vtbl= {
  nr_ice_crypto_dummy_random_bytes,
  nr_ice_crypto_dummy_hmac_sha1
};



nr_ice_crypto_vtbl *nr_crypto_vtbl=&nr_ice_crypto_dummy_vtbl;


