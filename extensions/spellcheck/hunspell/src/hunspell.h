#ifndef _MYSPELLMGR_H_
#define _MYSPELLMGR_H_

#include "hunvisapi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hunhandle Hunhandle;

LIBHUNSPELL_DLL_EXPORTED Hunhandle *Hunspell_create(const char * affpath, const char * dpath);

LIBHUNSPELL_DLL_EXPORTED Hunhandle *Hunspell_create_key(const char * affpath, const char * dpath,
    const char * key);

LIBHUNSPELL_DLL_EXPORTED void Hunspell_destroy(Hunhandle *pHunspell);




LIBHUNSPELL_DLL_EXPORTED int Hunspell_spell(Hunhandle *pHunspell, const char *);

LIBHUNSPELL_DLL_EXPORTED char *Hunspell_get_dic_encoding(Hunhandle *pHunspell);








LIBHUNSPELL_DLL_EXPORTED int Hunspell_suggest(Hunhandle *pHunspell, char*** slst, const char * word);

 

 

LIBHUNSPELL_DLL_EXPORTED int Hunspell_analyze(Hunhandle *pHunspell, char*** slst, const char * word);

 

LIBHUNSPELL_DLL_EXPORTED int Hunspell_stem(Hunhandle *pHunspell, char*** slst, const char * word);

 






LIBHUNSPELL_DLL_EXPORTED int Hunspell_stem2(Hunhandle *pHunspell, char*** slst, char** desc, int n);

 

LIBHUNSPELL_DLL_EXPORTED int Hunspell_generate(Hunhandle *pHunspell, char*** slst, const char * word,
    const char * word2);

 







LIBHUNSPELL_DLL_EXPORTED int Hunspell_generate2(Hunhandle *pHunspell, char*** slst, const char * word,
    char** desc, int n);

  

  
  
LIBHUNSPELL_DLL_EXPORTED int Hunspell_add(Hunhandle *pHunspell, const char * word);

  



  
LIBHUNSPELL_DLL_EXPORTED int Hunspell_add_with_affix(Hunhandle *pHunspell, const char * word, const char * example);

  

LIBHUNSPELL_DLL_EXPORTED int Hunspell_remove(Hunhandle *pHunspell, const char * word);

  

LIBHUNSPELL_DLL_EXPORTED void Hunspell_free_list(Hunhandle *pHunspell, char *** slst, int n);

#ifdef __cplusplus
}
#endif

#endif
