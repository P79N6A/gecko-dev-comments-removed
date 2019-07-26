#include "hunvisapi.h"

#include "hashmgr.hxx"
#include "affixmgr.hxx"
#include "suggestmgr.hxx"
#include "langnum.hxx"

#define  SPELL_XML "<?xml?>"

#define MAXDIC 20
#define MAXSUGGESTION 15
#define MAXSHARPS 5

#define HUNSPELL_OK       (1 << 0)
#define HUNSPELL_OK_WARN  (1 << 1)

#ifndef _MYSPELLMGR_HXX_
#define _MYSPELLMGR_HXX_

class LIBHUNSPELL_DLL_EXPORTED Hunspell
{
private:
  Hunspell(const Hunspell&);
  Hunspell& operator = (const Hunspell&);
private:
  AffixMgr*       pAMgr;
  HashMgr*        pHMgr[MAXDIC];
  int             maxdic;
  SuggestMgr*     pSMgr;
  char *          affixpath;
  char *          encoding;
  struct cs_info * csconv;
  int             langnum;
  int             utf8;
  int             complexprefixes;
  char**          wordbreak;

public:

  








  Hunspell(const char * affpath, const char * dpath, const char * key = NULL);
  ~Hunspell();

  
  int add_dic(const char * dpath, const char * key = NULL);

  








   
  int spell(const char * word, int * info = NULL, char ** root = NULL);

  







  int suggest(char*** slst, const char * word);

  

  void free_list(char *** slst, int n);

  char * get_dic_encoding();

 

 
 
  int analyze(char*** slst, const char * word);

 
  
  int stem(char*** slst, const char * word);
  
 





 
  int stem(char*** slst, char ** morph, int n);

 

  int generate(char*** slst, const char * word, const char * word2);

 







  int generate(char*** slst, const char * word, char ** desc, int n);

  

  
  
  int add(const char * word);

  



  
  int add_with_affix(const char * word, const char * example);

  

  int remove(const char * word);

  

  
  const char * get_wordchars();
  unsigned short * get_wordchars_utf16(int * len);

  struct cs_info * get_csconv();
  const char * get_version();

  int get_langnum() const;

  
  int input_conv(const char * word, char * dest);
  
  

#ifdef HUNSPELL_EXPERIMENTAL
    
  int put_word_suffix(const char * word, const char * suffix);
  char * morph_with_correction(const char * word);

  
  int suggest_auto(char*** slst, const char * word);
  int suggest_pos_stems(char*** slst, const char * word);
#endif

private:
   int    cleanword(char *, const char *, int * pcaptype, int * pabbrev);
   int    cleanword2(char *, const char *, w_char *, int * w_len, int * pcaptype, int * pabbrev);
   void   mkinitcap(char *);
   int    mkinitcap2(char * p, w_char * u, int nc);
   int    mkinitsmall2(char * p, w_char * u, int nc);
   void   mkallcap(char *);
   int    mkallcap2(char * p, w_char * u, int nc);
   void   mkallsmall(char *);
   int    mkallsmall2(char * p, w_char * u, int nc);
   struct hentry * checkword(const char *, int * info, char **root);
   char * sharps_u8_l1(char * dest, char * source);
   hentry * spellsharps(char * base, char *, int, int, char * tmp, int * info, char **root);
   int    is_keepcase(const hentry * rv);
   int    insert_sug(char ***slst, char * word, int ns);
   void   cat_result(char * result, char * st);
   char * stem_description(const char * desc);
   int    spellml(char*** slst, const char * word);
   int    get_xml_par(char * dest, const char * par, int maxl);
   const char * get_xml_pos(const char * s, const char * attr);
   int    get_xml_list(char ***slst, char * list, const char * tag);
   int    check_xml_par(const char * q, const char * attr, const char * value);

};

#endif
