




























#ifndef __PHONETHXX__
#define __PHONETHXX__

#define HASHSIZE          256
#define MAXPHONETLEN      256
#define MAXPHONETUTF8LEN  (MAXPHONETLEN * 4)

#include "hunvisapi.h"

struct phonetable {
  char utf8;
  cs_info * lang;
  int num;
  char * * rules;
  int hash[HASHSIZE];
};

LIBHUNSPELL_DLL_EXPORTED void init_phonet_hash(phonetable & parms);

LIBHUNSPELL_DLL_EXPORTED int phonet (const char * inword, char * target,
              int len, phonetable & phone);

#endif
