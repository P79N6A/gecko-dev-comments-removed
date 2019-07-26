#include "license.hunspell"
#include "license.myspell"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <vector>

#include "affixmgr.hxx"
#include "affentry.hxx"
#include "langnum.hxx"

#include "csutil.hxx"

AffixMgr::AffixMgr(const char * affpath, HashMgr** ptr, int * md, const char * key) 
{
  
  pHMgr = ptr[0];
  alldic = ptr;
  maxdic = md;
  keystring = NULL;
  trystring = NULL;
  encoding=NULL;
  csconv=NULL;
  utf8 = 0;
  complexprefixes = 0;
  maptable = NULL;
  nummap = 0;
  breaktable = NULL;
  numbreak = -1;
  reptable = NULL;
  numrep = 0;
  iconvtable = NULL;
  oconvtable = NULL;
  checkcpdtable = NULL;
  
  simplifiedcpd = 0;
  numcheckcpd = 0;
  defcpdtable = NULL;
  numdefcpd = 0;
  phone = NULL;
  compoundflag = FLAG_NULL; 
  compoundbegin = FLAG_NULL; 
  compoundmiddle = FLAG_NULL; 
  compoundend = FLAG_NULL; 
  compoundroot = FLAG_NULL; 
  compoundpermitflag = FLAG_NULL; 
  compoundforbidflag = FLAG_NULL; 
  compoundmoresuffixes = 0; 
  checkcompounddup = 0; 
  checkcompoundrep = 0; 
  checkcompoundcase = 0; 
  checkcompoundtriple = 0; 
  simplifiedtriple = 0; 
  forbiddenword = FORBIDDENWORD; 
  nosuggest = FLAG_NULL; 
  nongramsuggest = FLAG_NULL;
  lang = NULL; 
  langnum = 0; 
  needaffix = FLAG_NULL; 
  cpdwordmax = -1; 
  cpdmin = -1;  
  cpdmaxsyllable = 0; 
  cpdvowels=NULL; 
  cpdvowels_utf16=NULL; 
  cpdvowels_utf16_len=0; 
  pfxappnd=NULL; 
  sfxappnd=NULL; 
  cpdsyllablenum=NULL; 
  checknum=0; 
  wordchars=NULL; 
  wordchars_utf16=NULL; 
  wordchars_utf16_len=0; 
  ignorechars=NULL; 
  ignorechars_utf16=NULL; 
  ignorechars_utf16_len=0; 
  version=NULL; 
  havecontclass=0; 
  
  
  lemma_present = FLAG_NULL; 
  circumfix = FLAG_NULL; 
  onlyincompound = FLAG_NULL; 
  maxngramsugs = -1; 
  maxdiff = -1; 
  onlymaxdiff = 0;
  maxcpdsugs = -1; 
  nosplitsugs = 0;
  sugswithdots = 0;
  keepcase = 0;
  forceucase = 0;
  warn = 0;
  forbidwarn = 0;
  checksharps = 0;
  substandard = FLAG_NULL;
  fullstrip = 0;

  sfx = NULL;
  pfx = NULL;

  for (int i=0; i < SETSIZE; i++) {
     pStart[i] = NULL;
     sStart[i] = NULL;
     pFlag[i] = NULL;
     sFlag[i] = NULL;
  }

  for (int j=0; j < CONTSIZE; j++) {
    contclasses[j] = 0;
  }

  if (parse_file(affpath, key)) {
     HUNSPELL_WARNING(stderr, "Failure loading aff file %s\n",affpath);
  }
  
  if (cpdmin == -1) cpdmin = MINCPDLEN;

}


AffixMgr::~AffixMgr() 
{
  
  for (int i=0; i < SETSIZE ;i++) {
       pFlag[i] = NULL;
       PfxEntry * ptr = pStart[i];
       PfxEntry * nptr = NULL;
       while (ptr) {
            nptr = ptr->getNext();
            delete(ptr);
            ptr = nptr;
            nptr = NULL;
       }  
  }

  
  for (int j=0; j < SETSIZE ; j++) {
       sFlag[j] = NULL;
       SfxEntry * ptr = sStart[j];
       SfxEntry * nptr = NULL;
       while (ptr) {
            nptr = ptr->getNext();
            delete(ptr);
            ptr = nptr;
            nptr = NULL;
       }
       sStart[j] = NULL;
  }

  if (keystring) free(keystring);
  keystring=NULL;
  if (trystring) free(trystring);
  trystring=NULL;
  if (encoding) free(encoding);
  encoding=NULL;
  if (maptable) {  
     for (int j=0; j < nummap; j++) {
        for (int k=0; k < maptable[j].len; k++) {
           if (maptable[j].set[k]) free(maptable[j].set[k]);
        }
        free(maptable[j].set);
        maptable[j].set = NULL;
        maptable[j].len = 0;
     }
     free(maptable);  
     maptable = NULL;
  }
  nummap = 0;
  if (breaktable) {
     for (int j=0; j < numbreak; j++) {
        if (breaktable[j]) free(breaktable[j]);
        breaktable[j] = NULL;
     }
     free(breaktable);  
     breaktable = NULL;
  }
  numbreak = 0;
  if (reptable) {
     for (int j=0; j < numrep; j++) {
        free(reptable[j].pattern);
        free(reptable[j].pattern2);
     }
     free(reptable);  
     reptable = NULL;
  }
  if (iconvtable) delete iconvtable;
  if (oconvtable) delete oconvtable;
  if (phone && phone->rules) {
     for (int j=0; j < phone->num + 1; j++) {
        free(phone->rules[j * 2]);
        free(phone->rules[j * 2 + 1]);
     }
     free(phone->rules);
     free(phone);  
     phone = NULL;
  }

  if (defcpdtable) {  
     for (int j=0; j < numdefcpd; j++) {
        free(defcpdtable[j].def);
        defcpdtable[j].def = NULL;
     }
     free(defcpdtable);  
     defcpdtable = NULL;
  }
  numrep = 0;
  if (checkcpdtable) {  
     for (int j=0; j < numcheckcpd; j++) {
        free(checkcpdtable[j].pattern);
        free(checkcpdtable[j].pattern2);
        free(checkcpdtable[j].pattern3);
        checkcpdtable[j].pattern = NULL;
        checkcpdtable[j].pattern2 = NULL;
        checkcpdtable[j].pattern3 = NULL;
     }
     free(checkcpdtable);  
     checkcpdtable = NULL;
  }
  numcheckcpd = 0;
  FREE_FLAG(compoundflag);
  FREE_FLAG(compoundbegin);
  FREE_FLAG(compoundmiddle);
  FREE_FLAG(compoundend);
  FREE_FLAG(compoundpermitflag);
  FREE_FLAG(compoundforbidflag);
  FREE_FLAG(compoundroot);
  FREE_FLAG(forbiddenword);
  FREE_FLAG(nosuggest);
  FREE_FLAG(nongramsuggest);
  FREE_FLAG(needaffix);
  FREE_FLAG(lemma_present);
  FREE_FLAG(circumfix);
  FREE_FLAG(onlyincompound);
  
  cpdwordmax = 0;
  pHMgr = NULL;
  cpdmin = 0;
  cpdmaxsyllable = 0;
  if (cpdvowels) free(cpdvowels);
  if (cpdvowels_utf16) free(cpdvowels_utf16);
  if (cpdsyllablenum) free(cpdsyllablenum);
  free_utf_tbl();
  if (lang) free(lang);
  if (wordchars) free(wordchars);
  if (wordchars_utf16) free(wordchars_utf16);
  if (ignorechars) free(ignorechars);
  if (ignorechars_utf16) free(ignorechars_utf16);
  if (version) free(version);
  checknum=0;
#ifdef MOZILLA_CLIENT
  delete [] csconv;
#endif
}

void AffixMgr::finishFileMgr(FileMgr *afflst)
{
    delete afflst;

    
    process_pfx_tree_to_list();
    process_sfx_tree_to_list();
}


int  AffixMgr::parse_file(const char * affpath, const char * key)
{
  char * line; 
  char ft;     
  
  
  char dupflags[CONTSIZE];
  char dupflags_ini = 1;

  
  int firstline = 1;
  
  
  FileMgr * afflst = new FileMgr(affpath, key);
  if (!afflst) {
    HUNSPELL_WARNING(stderr, "error: could not open affix description file %s\n",affpath);
    return 1;
  }

  
  

    
    
    while ((line = afflst->getline()) != NULL) {
       mychomp(line);

       
       if (firstline) {
         firstline = 0;
         
         if (strncmp(line,"\xEF\xBB\xBF",3) == 0) {
            memmove(line, line+3, strlen(line+3)+1);
         }
       }

       
       if (strncmp(line,"KEY",3) == 0) {
          if (parse_string(line, &keystring, afflst->getlinenum())) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"TRY",3) == 0) {
          if (parse_string(line, &trystring, afflst->getlinenum())) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"SET",3) == 0) {
          if (parse_string(line, &encoding, afflst->getlinenum())) {
             finishFileMgr(afflst);
             return 1;
          }
          if (strcmp(encoding, "UTF-8") == 0) {
             utf8 = 1;
#ifndef OPENOFFICEORG
#ifndef MOZILLA_CLIENT
             if (initialize_utf_tbl()) return 1;
#endif
#endif
          }
       }

       
       if (strncmp(line,"COMPLEXPREFIXES",15) == 0)
                   complexprefixes = 1;

       
       if (strncmp(line,"COMPOUNDFLAG",12) == 0) {
          if (parse_flag(line, &compoundflag, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"COMPOUNDBEGIN",13) == 0) {
          if (complexprefixes) {
            if (parse_flag(line, &compoundend, afflst)) {
              finishFileMgr(afflst);
              return 1;
            }
          } else {
            if (parse_flag(line, &compoundbegin, afflst)) {
              finishFileMgr(afflst);
              return 1;
            }
          }
       }

       
       if (strncmp(line,"COMPOUNDMIDDLE",14) == 0) {
          if (parse_flag(line, &compoundmiddle, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }
       
       if (strncmp(line,"COMPOUNDEND",11) == 0) {
          if (complexprefixes) {
            if (parse_flag(line, &compoundbegin, afflst)) {
              finishFileMgr(afflst);
              return 1;
            }
          } else {
            if (parse_flag(line, &compoundend, afflst)) {
              finishFileMgr(afflst);
              return 1;
            }
          }
       }

       
       if (strncmp(line,"COMPOUNDWORDMAX",15) == 0) {
          if (parse_num(line, &cpdwordmax, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"COMPOUNDROOT",12) == 0) {
          if (parse_flag(line, &compoundroot, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"COMPOUNDPERMITFLAG",18) == 0) {
          if (parse_flag(line, &compoundpermitflag, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"COMPOUNDFORBIDFLAG",18) == 0) {
          if (parse_flag(line, &compoundforbidflag, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       if (strncmp(line,"COMPOUNDMORESUFFIXES",20) == 0) {
                   compoundmoresuffixes = 1;
       }

       if (strncmp(line,"CHECKCOMPOUNDDUP",16) == 0) {
                   checkcompounddup = 1;
       }

       if (strncmp(line,"CHECKCOMPOUNDREP",16) == 0) {
                   checkcompoundrep = 1;
       }

       if (strncmp(line,"CHECKCOMPOUNDTRIPLE",19) == 0) {
                   checkcompoundtriple = 1;
       }

       if (strncmp(line,"SIMPLIFIEDTRIPLE",16) == 0) {
                   simplifiedtriple = 1;
       }

       if (strncmp(line,"CHECKCOMPOUNDCASE",17) == 0) {
                   checkcompoundcase = 1;
       }

       if (strncmp(line,"NOSUGGEST",9) == 0) {
          if (parse_flag(line, &nosuggest, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       if (strncmp(line,"NONGRAMSUGGEST",14) == 0) {
          if (parse_flag(line, &nongramsuggest, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"FORBIDDENWORD",13) == 0) {
          if (parse_flag(line, &forbiddenword, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"LEMMA_PRESENT",13) == 0) {
          if (parse_flag(line, &lemma_present, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"CIRCUMFIX",9) == 0) {
          if (parse_flag(line, &circumfix, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"ONLYINCOMPOUND",14) == 0) {
          if (parse_flag(line, &onlyincompound, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"PSEUDOROOT",10) == 0) {
          if (parse_flag(line, &needaffix, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"NEEDAFFIX",9) == 0) {
          if (parse_flag(line, &needaffix, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"COMPOUNDMIN",11) == 0) {
          if (parse_num(line, &cpdmin, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
          if (cpdmin < 1) cpdmin = 1;
       }

       
       if (strncmp(line,"COMPOUNDSYLLABLE",16) == 0) {
          if (parse_cpdsyllable(line, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"SYLLABLENUM",11) == 0) {
          if (parse_string(line, &cpdsyllablenum, afflst->getlinenum())) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"CHECKNUM",8) == 0) {
           checknum=1;
       }

       
       if (strncmp(line,"WORDCHARS",9) == 0) {
          if (parse_array(line, &wordchars, &wordchars_utf16, &wordchars_utf16_len, utf8, afflst->getlinenum())) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"IGNORE",6) == 0) {
          if (parse_array(line, &ignorechars, &ignorechars_utf16, &ignorechars_utf16_len, utf8, afflst->getlinenum())) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"REP",3) == 0) {
          if (parse_reptable(line, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"ICONV",5) == 0) {
          if (parse_convtable(line, afflst, &iconvtable, "ICONV")) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"OCONV",5) == 0) {
          if (parse_convtable(line, afflst, &oconvtable, "OCONV")) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"PHONE",5) == 0) {
          if (parse_phonetable(line, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"CHECKCOMPOUNDPATTERN",20) == 0) {
          if (parse_checkcpdtable(line, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"COMPOUNDRULE",12) == 0) {
          if (parse_defcpdtable(line, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"MAP",3) == 0) {
          if (parse_maptable(line, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"BREAK",5) == 0) {
          if (parse_breaktable(line, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"LANG",4) == 0) {
          if (parse_string(line, &lang, afflst->getlinenum())) {
             finishFileMgr(afflst);
             return 1;
          }
          langnum = get_lang_num(lang);
       }

       if (strncmp(line,"VERSION",7) == 0) {
          for(line = line + 7; *line == ' ' || *line == '\t'; line++);
          version = mystrdup(line);
       }

       if (strncmp(line,"MAXNGRAMSUGS",12) == 0) {
          if (parse_num(line, &maxngramsugs, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       if (strncmp(line,"ONLYMAXDIFF", 11) == 0)
                   onlymaxdiff = 1;

       if (strncmp(line,"MAXDIFF",7) == 0) {
          if (parse_num(line, &maxdiff, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       if (strncmp(line,"MAXCPDSUGS",10) == 0) {
          if (parse_num(line, &maxcpdsugs, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       if (strncmp(line,"NOSPLITSUGS",11) == 0) {
                   nosplitsugs=1;
       }

       if (strncmp(line,"FULLSTRIP",9) == 0) {
                   fullstrip=1;
       }

       if (strncmp(line,"SUGSWITHDOTS",12) == 0) {
                   sugswithdots=1;
       }

       
       if (strncmp(line,"KEEPCASE",8) == 0) {
          if (parse_flag(line, &keepcase, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"FORCEUCASE",10) == 0) {
          if (parse_flag(line, &forceucase, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       
       if (strncmp(line,"WARN",4) == 0) {
          if (parse_flag(line, &warn, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       if (strncmp(line,"FORBIDWARN",10) == 0) {
                   forbidwarn=1;
       }

       
       if (strncmp(line,"SUBSTANDARD",11) == 0) {
          if (parse_flag(line, &substandard, afflst)) {
             finishFileMgr(afflst);
             return 1;
          }
       }

       if (strncmp(line,"CHECKSHARPS",11) == 0) {
                   checksharps=1;
       }

       
       ft = ' ';
       if (strncmp(line,"PFX",3) == 0) ft = complexprefixes ? 'S' : 'P';
       if (strncmp(line,"SFX",3) == 0) ft = complexprefixes ? 'P' : 'S';
       if (ft != ' ') {
          if (dupflags_ini) {
            memset(dupflags, 0, sizeof(dupflags));
            dupflags_ini = 0;
          }
          if (parse_affix(line, ft, afflst, dupflags)) {
             finishFileMgr(afflst);
             return 1;
          }
       }
    }

    finishFileMgr(afflst);
    

    
    

    
    

    
    
    
    
    

    
    
    
 
    
    
    

    
    

    process_pfx_order();
    process_sfx_order();

    
    if (!utf8) {
    char * enc = get_encoding();
    csconv = get_current_cs(enc);
    free(enc);
    enc = NULL;

    char expw[MAXLNLEN];
    if (wordchars) {
        strcpy(expw, wordchars);
        free(wordchars);
    } else *expw = '\0';

    for (int i = 0; i <= 255; i++) {
        if ( (csconv[i].cupper != csconv[i].clower) &&
            (! strchr(expw, (char) i))) {
                *(expw + strlen(expw) + 1) = '\0';
                *(expw + strlen(expw)) = (char) i;
        }
    }

    wordchars = mystrdup(expw);
    }

    
    if (numbreak == -1) {
        breaktable = (char **) malloc(sizeof(char *) * 3);
        if (!breaktable) return 1;
        breaktable[0] = mystrdup("-");
        breaktable[1] = mystrdup("^-");
        breaktable[2] = mystrdup("-$");
        if (breaktable[0] && breaktable[1] && breaktable[2]) numbreak = 3;
    }
    return 0;
}






int AffixMgr::build_pfxtree(PfxEntry* pfxptr)
{
  PfxEntry * ptr;
  PfxEntry * pptr;
  PfxEntry * ep = pfxptr;

  
  const char * key = ep->getKey();
  const unsigned char flg = (unsigned char) (ep->getFlag() & 0x00FF);

  
  ptr = pFlag[flg];
  ep->setFlgNxt(ptr);
  pFlag[flg] = ep;


  
  if (strlen(key) == 0) {
    
     ptr = pStart[0];
     ep->setNext(ptr);
     pStart[0] = ep;
     return 0;
  }

  
  ep->setNextEQ(NULL);
  ep->setNextNE(NULL);

  unsigned char sp = *((const unsigned char *)key);
  ptr = pStart[sp];
  
  
  if (!ptr) {
     pStart[sp] = ep;
     return 0;
  }


  
  
  pptr = NULL;
  for (;;) {
    pptr = ptr;
    if (strcmp(ep->getKey(), ptr->getKey() ) <= 0) {
       ptr = ptr->getNextEQ();
       if (!ptr) {
          pptr->setNextEQ(ep);
          break;
       }
    } else {
       ptr = ptr->getNextNE();
       if (!ptr) {
          pptr->setNextNE(ep);
          break;
       }
    }
  }
  return 0;
}




int AffixMgr::build_sfxtree(SfxEntry* sfxptr)
{
  SfxEntry * ptr;
  SfxEntry * pptr;
  SfxEntry * ep = sfxptr;

  
  const char * key = ep->getKey();
  const unsigned char flg = (unsigned char) (ep->getFlag() & 0x00FF);

  
  ptr = sFlag[flg];
  ep->setFlgNxt(ptr);
  sFlag[flg] = ep;

  

  
  if (strlen(key) == 0) {
    
     ptr = sStart[0];
     ep->setNext(ptr);
     sStart[0] = ep;
     return 0;
  }

  
  ep->setNextEQ(NULL);
  ep->setNextNE(NULL);

  unsigned char sp = *((const unsigned char *)key);
  ptr = sStart[sp];
  
  
  if (!ptr) {
     sStart[sp] = ep;
     return 0;
  }

  
  
  pptr = NULL;
  for (;;) {
    pptr = ptr;
    if (strcmp(ep->getKey(), ptr->getKey() ) <= 0) {
       ptr = ptr->getNextEQ();
       if (!ptr) {
          pptr->setNextEQ(ep);
          break;
       }
    } else {
       ptr = ptr->getNextNE();
       if (!ptr) {
          pptr->setNextNE(ep);
          break;
       }
    }
  }
  return 0;
}


int AffixMgr::process_pfx_tree_to_list()
{
  for (int i=1; i< SETSIZE; i++) {
    pStart[i] = process_pfx_in_order(pStart[i],NULL);
  }
  return 0;
}


PfxEntry* AffixMgr::process_pfx_in_order(PfxEntry* ptr, PfxEntry* nptr)
{
  if (ptr) {
    nptr = process_pfx_in_order(ptr->getNextNE(), nptr);
    ptr->setNext(nptr);
    nptr = process_pfx_in_order(ptr->getNextEQ(), ptr);
  }
  return nptr;
}



int AffixMgr:: process_sfx_tree_to_list()
{
  for (int i=1; i< SETSIZE; i++) {
    sStart[i] = process_sfx_in_order(sStart[i],NULL);
  }
  return 0;
}

SfxEntry* AffixMgr::process_sfx_in_order(SfxEntry* ptr, SfxEntry* nptr)
{
  if (ptr) {
    nptr = process_sfx_in_order(ptr->getNextNE(), nptr);
    ptr->setNext(nptr);
    nptr = process_sfx_in_order(ptr->getNextEQ(), ptr);
  }
  return nptr;
}




int AffixMgr::process_pfx_order()
{
    PfxEntry* ptr;

    
    for (int i=1; i < SETSIZE; i++) {

         ptr = pStart[i];

         
         
         
         
         
         

         for (; ptr != NULL; ptr = ptr->getNext()) {

             PfxEntry * nptr = ptr->getNext();
             for (; nptr != NULL; nptr = nptr->getNext()) {
                 if (! isSubset( ptr->getKey() , nptr->getKey() )) break;
             }
             ptr->setNextNE(nptr);
             ptr->setNextEQ(NULL);
             if ((ptr->getNext()) && isSubset(ptr->getKey() , (ptr->getNext())->getKey())) 
                 ptr->setNextEQ(ptr->getNext());
         }

         
         
         
         

         ptr = pStart[i];
         for (; ptr != NULL; ptr = ptr->getNext()) {
             PfxEntry * nptr = ptr->getNext();
             PfxEntry * mptr = NULL;
             for (; nptr != NULL; nptr = nptr->getNext()) {
                 if (! isSubset(ptr->getKey(),nptr->getKey())) break;
                 mptr = nptr;
             }
             if (mptr) mptr->setNextNE(NULL);
         }
    }
    return 0;
}



int AffixMgr::process_sfx_order()
{
    SfxEntry* ptr;

    
    for (int i=1; i < SETSIZE; i++) {

         ptr = sStart[i];

         
         
         
         
         
         

         for (; ptr != NULL; ptr = ptr->getNext()) {
             SfxEntry * nptr = ptr->getNext();
             for (; nptr != NULL; nptr = nptr->getNext()) {
                 if (! isSubset(ptr->getKey(),nptr->getKey())) break;
             }
             ptr->setNextNE(nptr);
             ptr->setNextEQ(NULL);
             if ((ptr->getNext()) && isSubset(ptr->getKey(),(ptr->getNext())->getKey())) 
                 ptr->setNextEQ(ptr->getNext());
         }


         
         
         
         

         ptr = sStart[i];
         for (; ptr != NULL; ptr = ptr->getNext()) {
             SfxEntry * nptr = ptr->getNext();
             SfxEntry * mptr = NULL;
             for (; nptr != NULL; nptr = nptr->getNext()) {
                 if (! isSubset(ptr->getKey(),nptr->getKey())) break;
                 mptr = nptr;
             }
             if (mptr) mptr->setNextNE(NULL);
         }
    }
    return 0;
}


void AffixMgr::debugflag(char * result, unsigned short flag) {
    char * st = encode_flag(flag);
    mystrcat(result, " ", MAXLNLEN);
    mystrcat(result, MORPH_FLAG, MAXLNLEN);
    if (st) {
        mystrcat(result, st, MAXLNLEN);
        free(st);
    }
}


int AffixMgr::condlen(char * st)
{
  int l = 0;
  bool group = false;
  for(; *st; st++) {
    if (*st == '[') {
        group = true;
        l++;
    } else if (*st == ']') group = false;
    else if (!group && (!utf8 ||
        (!(*st & 0x80) || ((*st & 0xc0) == 0x80)))) l++;
  }
  return l;
}

int AffixMgr::encodeit(affentry &entry, char * cs)
{
  if (strcmp(cs,".") != 0) {
    entry.numconds = (char) condlen(cs);
    strncpy(entry.c.conds, cs, MAXCONDLEN);
    
    if (entry.c.conds[MAXCONDLEN - 1] && cs[MAXCONDLEN]) {
      entry.opts += aeLONGCOND;
      entry.c.l.conds2 = mystrdup(cs + MAXCONDLEN_1);
      if (!entry.c.l.conds2) return 1;
    }
  } else {
    entry.numconds = 0;
    entry.c.conds[0] = '\0';
  }
  return 0;
}


inline int AffixMgr::isSubset(const char * s1, const char * s2)
 {
    while (((*s1 == *s2) || (*s1 == '.')) && (*s1 != '\0')) {
        s1++;
        s2++;
    }
    return (*s1 == '\0');
 }



struct hentry * AffixMgr::prefix_check(const char * word, int len, char in_compound,
    const FLAG needflag)
{
    struct hentry * rv= NULL;

    pfx = NULL;
    pfxappnd = NULL;
    sfxappnd = NULL;
    
    
    PfxEntry * pe = pStart[0];
    while (pe) {
        if (
            
              ((in_compound != IN_CPD_NOT) || !(pe->getCont() &&
                  (TESTAFF(pe->getCont(), onlyincompound, pe->getContLen())))) &&
            
              ((in_compound != IN_CPD_END) || (pe->getCont() &&
                  (TESTAFF(pe->getCont(), compoundpermitflag, pe->getContLen()))))
              ) {
                    
                    rv = pe->checkword(word, len, in_compound, needflag);
                    if (rv) {
                        pfx=pe; 
                        return rv;
                    }
             }
       pe = pe->getNext();
    }
  
    
    unsigned char sp = *((const unsigned char *)word);
    PfxEntry * pptr = pStart[sp];

    while (pptr) {
        if (isSubset(pptr->getKey(),word)) {
             if (
            
              ((in_compound != IN_CPD_NOT) || !(pptr->getCont() &&
                  (TESTAFF(pptr->getCont(), onlyincompound, pptr->getContLen())))) &&
            
              ((in_compound != IN_CPD_END) || (pptr->getCont() &&
                  (TESTAFF(pptr->getCont(), compoundpermitflag, pptr->getContLen()))))
              ) {
            
                  rv = pptr->checkword(word, len, in_compound, needflag);
                  if (rv) {
                    pfx=pptr; 
                    return rv;
                  }
             }
             pptr = pptr->getNextEQ();
        } else {
             pptr = pptr->getNextNE();
        }
    }
    
    return NULL;
}


struct hentry * AffixMgr::prefix_check_twosfx(const char * word, int len,
    char in_compound, const FLAG needflag)
{
    struct hentry * rv= NULL;

    pfx = NULL;
    sfxappnd = NULL;
    
    
    PfxEntry * pe = pStart[0];
    
    while (pe) {
        rv = pe->check_twosfx(word, len, in_compound, needflag);
        if (rv) return rv;
        pe = pe->getNext();
    }
  
    
    unsigned char sp = *((const unsigned char *)word);
    PfxEntry * pptr = pStart[sp];

    while (pptr) {
        if (isSubset(pptr->getKey(),word)) {
            rv = pptr->check_twosfx(word, len, in_compound, needflag);
            if (rv) {
                pfx = pptr;
                return rv;
            }
            pptr = pptr->getNextEQ();
        } else {
             pptr = pptr->getNextNE();
        }
    }
    
    return NULL;
}


char * AffixMgr::prefix_check_morph(const char * word, int len, char in_compound,
    const FLAG needflag)
{
    char * st;

    char result[MAXLNLEN];
    result[0] = '\0';

    pfx = NULL;
    sfxappnd = NULL;
    
    
    PfxEntry * pe = pStart[0];
    while (pe) {
       st = pe->check_morph(word,len,in_compound, needflag);
       if (st) {
            mystrcat(result, st, MAXLNLEN);
            free(st);
       }
       
       pe = pe->getNext();
    }
  
    
    unsigned char sp = *((const unsigned char *)word);
    PfxEntry * pptr = pStart[sp];

    while (pptr) {
        if (isSubset(pptr->getKey(),word)) {
            st = pptr->check_morph(word,len,in_compound, needflag);
            if (st) {
              
              if ((in_compound != IN_CPD_NOT) || !((pptr->getCont() && 
                        (TESTAFF(pptr->getCont(), onlyincompound, pptr->getContLen()))))) {
                    mystrcat(result, st, MAXLNLEN);
                    pfx = pptr;
                }
                free(st);
            }
            pptr = pptr->getNextEQ();
        } else {
            pptr = pptr->getNextNE();
        }
    }
    
    if (*result) return mystrdup(result);
    return NULL;
}



char * AffixMgr::prefix_check_twosfx_morph(const char * word, int len,
    char in_compound, const FLAG needflag)
{
    char * st;

    char result[MAXLNLEN];
    result[0] = '\0';

    pfx = NULL;
    sfxappnd = NULL;
    
    
    PfxEntry * pe = pStart[0];
    while (pe) {
        st = pe->check_twosfx_morph(word,len,in_compound, needflag);
        if (st) {
            mystrcat(result, st, MAXLNLEN);
            free(st);
        }
        pe = pe->getNext();
    }
  
    
    unsigned char sp = *((const unsigned char *)word);
    PfxEntry * pptr = pStart[sp];

    while (pptr) {
        if (isSubset(pptr->getKey(),word)) {
            st = pptr->check_twosfx_morph(word, len, in_compound, needflag);
            if (st) {
                mystrcat(result, st, MAXLNLEN);
                free(st);
                pfx = pptr;
            }
            pptr = pptr->getNextEQ();
        } else {
            pptr = pptr->getNextNE();
        }
    }
    
    if (*result) return mystrdup(result);
    return NULL;
}


int AffixMgr::cpdrep_check(const char * word, int wl)
{
  char candidate[MAXLNLEN];
  const char * r;
  int lenr, lenp;

  if ((wl < 2) || !numrep) return 0;

  for (int i=0; i < numrep; i++ ) {
      r = word;
      lenr = strlen(reptable[i].pattern2);
      lenp = strlen(reptable[i].pattern);
      
      while ((r=strstr(r, reptable[i].pattern)) != NULL) {
          strcpy(candidate, word);
          if (r-word + lenr + strlen(r+lenp) >= MAXLNLEN) break;
          strcpy(candidate+(r-word),reptable[i].pattern2);
          strcpy(candidate+(r-word)+lenr, r+lenp);
          if (candidate_check(candidate,strlen(candidate))) return 1;
          r++; 
      }
   }
   return 0;
}


int AffixMgr::cpdpat_check(const char * word, int pos, hentry * r1, hentry * r2, const char )
{
  int len;
  for (int i = 0; i < numcheckcpd; i++) {
      if (isSubset(checkcpdtable[i].pattern2, word + pos) &&
        (!r1 || !checkcpdtable[i].cond ||
          (r1->astr && TESTAFF(r1->astr, checkcpdtable[i].cond, r1->alen))) &&
        (!r2 || !checkcpdtable[i].cond2 ||
          (r2->astr && TESTAFF(r2->astr, checkcpdtable[i].cond2, r2->alen))) &&
        
        
        (!*(checkcpdtable[i].pattern) || (
            (*(checkcpdtable[i].pattern)=='0' && r1->blen <= pos && strncmp(word + pos - r1->blen, r1->word, r1->blen) == 0) ||
            (*(checkcpdtable[i].pattern)!='0' && ((len = strlen(checkcpdtable[i].pattern)) != 0) &&
                strncmp(word + pos - len, checkcpdtable[i].pattern, len) == 0)))) {
            return 1;
        }
  }
  return 0;
}


int AffixMgr::cpdcase_check(const char * word, int pos)
{
  if (utf8) {
      w_char u, w;
      const char * p;
      u8_u16(&u, 1, word + pos);
      for (p = word + pos - 1; (*p & 0xc0) == 0x80; p--);
      u8_u16(&w, 1, p);
      unsigned short a = (u.h << 8) + u.l;
      unsigned short b = (w.h << 8) + w.l;
      if (((unicodetoupper(a, langnum) == a) || (unicodetoupper(b, langnum) == b)) &&
          (a != '-') && (b != '-')) return 1;
  } else {
      unsigned char a = *(word + pos - 1);
      unsigned char b = *(word + pos);
      if ((csconv[a].ccase || csconv[b].ccase) && (a != '-') && (b != '-')) return 1;
  }
  return 0;
}


int AffixMgr::defcpd_check(hentry *** words, short wnum, hentry * rv, hentry ** def, char all)
{
  signed short btpp[MAXWORDLEN]; 
  signed short btwp[MAXWORDLEN]; 
  int btnum[MAXWORDLEN]; 
  short bt = 0;  
  int i, j;
  int ok;
  int w = 0;

  if (!*words) {
    w = 1;
    *words = def;
  }

  if (!*words) {
    return 0;
  }

  (*words)[wnum] = rv;

  
  if (rv->alen == 0) {
    (*words)[wnum] = NULL;
    if (w) *words = NULL;
    return 0;
  }
  ok = 0;
  for (i = 0; i < numdefcpd; i++) {
    for (j = 0; j < defcpdtable[i].len; j++) {
       if (defcpdtable[i].def[j] != '*' && defcpdtable[i].def[j] != '?' &&
          TESTAFF(rv->astr, defcpdtable[i].def[j], rv->alen)) {
         ok = 1;
         break;
       }
    }
  }
  if (ok == 0) {
    (*words)[wnum] = NULL;
    if (w) *words = NULL;
    return 0;
  }

  for (i = 0; i < numdefcpd; i++) {
    signed short pp = 0; 
    signed short wp = 0; 
    int ok2;
    ok = 1;
    ok2 = 1;
    do {
      while ((pp < defcpdtable[i].len) && (wp <= wnum)) {
        if (((pp+1) < defcpdtable[i].len) &&
          ((defcpdtable[i].def[pp+1] == '*') || (defcpdtable[i].def[pp+1] == '?'))) {
            int wend = (defcpdtable[i].def[pp+1] == '?') ? wp : wnum;
            ok2 = 1;
            pp+=2;
            btpp[bt] = pp;
            btwp[bt] = wp;
            while (wp <= wend) {
                if (!(*words)[wp]->alen || 
                  !TESTAFF((*words)[wp]->astr, defcpdtable[i].def[pp-2], (*words)[wp]->alen)) {
                    ok2 = 0;
                    break;
                }
                wp++;
            }
            if (wp <= wnum) ok2 = 0;
            btnum[bt] = wp - btwp[bt];
            if (btnum[bt] > 0) bt++;
            if (ok2) break;
        } else {
            ok2 = 1;
            if (!(*words)[wp] || !(*words)[wp]->alen || 
              !TESTAFF((*words)[wp]->astr, defcpdtable[i].def[pp], (*words)[wp]->alen)) {
                ok = 0;
                break;
            }
            pp++;
            wp++;
            if ((defcpdtable[i].len == pp) && !(wp > wnum)) ok = 0;
        }
      }
    if (ok && ok2) { 
        int r = pp;
        while ((defcpdtable[i].len > r) && ((r+1) < defcpdtable[i].len) &&
            ((defcpdtable[i].def[r+1] == '*') || (defcpdtable[i].def[r+1] == '?'))) r+=2;
        if (defcpdtable[i].len <= r) return 1;
    }
    
    if (bt) do {
        ok = 1;
        btnum[bt - 1]--;
        pp = btpp[bt - 1];
        wp = btwp[bt - 1] + (signed short) btnum[bt - 1];
    } while ((btnum[bt - 1] < 0) && --bt);
  } while (bt);

  if (ok && ok2 && (!all || (defcpdtable[i].len <= pp))) return 1;

  
  while (ok && ok2 && (defcpdtable[i].len > pp) && ((pp+1) < defcpdtable[i].len) &&
    ((defcpdtable[i].def[pp+1] == '*') || (defcpdtable[i].def[pp+1] == '?'))) pp+=2;
  if (ok && ok2 && (defcpdtable[i].len <= pp)) return 1;
  }
  (*words)[wnum] = NULL;
  if (w) *words = NULL;
  return 0;
}

inline int AffixMgr::candidate_check(const char * word, int len)
{
  struct hentry * rv=NULL;
  
  rv = lookup(word);
  if (rv) return 1;



  
  rv = affix_check(word,len);
  if (rv) return 1;
  return 0;
}


short AffixMgr::get_syllable(const char * word, int wlen)
{
    if (cpdmaxsyllable==0) return 0;
    
    short num=0;

    if (!utf8) {
        for (int i=0; i<wlen; i++) {
            if (strchr(cpdvowels, word[i])) num++;
        }
    } else if (cpdvowels_utf16) {
        w_char w[MAXWORDUTF8LEN];
        int i = u8_u16(w, MAXWORDUTF8LEN, word);
        for (; i > 0; i--) {
            if (flag_bsearch((unsigned short *) cpdvowels_utf16,
                ((unsigned short *) w)[i - 1], cpdvowels_utf16_len)) num++;
        }
    }
    return num;
}

void AffixMgr::setcminmax(int * cmin, int * cmax, const char * word, int len) {
    if (utf8) {
        int i;
        for (*cmin = 0, i = 0; (i < cpdmin) && word[*cmin]; i++) {
          for ((*cmin)++; (word[*cmin] & 0xc0) == 0x80; (*cmin)++);
        }
        for (*cmax = len, i = 0; (i < (cpdmin - 1)) && *cmax; i++) {
          for ((*cmax)--; (word[*cmax] & 0xc0) == 0x80; (*cmax)--);
        }
    } else {
        *cmin = cpdmin;
        *cmax = len - cpdmin + 1;
    }
}




struct hentry * AffixMgr::compound_check(const char * word, int len, 
    short wordnum, short numsyllable, short maxwordnum, short wnum, hentry ** words = NULL,
    char hu_mov_rule = 0, char is_sug = 0, int * info = NULL)
{
    int i; 
    short oldnumsyllable, oldnumsyllable2, oldwordnum, oldwordnum2;
    struct hentry * rv = NULL;
    struct hentry * rv_first;
    struct hentry * rwords[MAXWORDLEN]; 
    char st [MAXWORDUTF8LEN + 4];
    char ch = '\0';
    int cmin;
    int cmax;
    int striple = 0;
    int scpd = 0;
    int soldi = 0;
    int oldcmin = 0;
    int oldcmax = 0;
    int oldlen = 0;
    int checkedstriple = 0;
    int onlycpdrule;
    char affixed = 0;
    hentry ** oldwords = words;

    int checked_prefix;

    setcminmax(&cmin, &cmax, word, len);

    strcpy(st, word);

    for (i = cmin; i < cmax; i++) {
        
        if (utf8) {
            for (; (st[i] & 0xc0) == 0x80; i++);
            if (i >= cmax) return NULL;
        }

        words = oldwords;
        onlycpdrule = (words) ? 1 : 0;

        do { 

        oldnumsyllable = numsyllable;
        oldwordnum = wordnum;
        checked_prefix = 0;


        do { 

        if (scpd > 0) {
          for (; scpd <= numcheckcpd && (!checkcpdtable[scpd-1].pattern3 ||
            strncmp(word + i, checkcpdtable[scpd-1].pattern3, strlen(checkcpdtable[scpd-1].pattern3)) != 0); scpd++);

          if (scpd > numcheckcpd) break; 
          strcpy(st + i, checkcpdtable[scpd-1].pattern);
          soldi = i;
          i += strlen(checkcpdtable[scpd-1].pattern);
          strcpy(st + i, checkcpdtable[scpd-1].pattern2);
          strcpy(st + i + strlen(checkcpdtable[scpd-1].pattern2), word + soldi + strlen(checkcpdtable[scpd-1].pattern3));

          oldlen = len;
          len += strlen(checkcpdtable[scpd-1].pattern) + strlen(checkcpdtable[scpd-1].pattern2) - strlen(checkcpdtable[scpd-1].pattern3);
          oldcmin = cmin;
          oldcmax = cmax;
          setcminmax(&cmin, &cmax, st, len);

          cmax = len - cpdmin + 1;
        }

        ch = st[i];
        st[i] = '\0';

        sfx = NULL;
        pfx = NULL;

        

        affixed = 1;
        rv = lookup(st); 

        
        while ((rv) && !hu_mov_rule &&
            ((needaffix && TESTAFF(rv->astr, needaffix, rv->alen)) ||
                !((compoundflag && !words && !onlycpdrule && TESTAFF(rv->astr, compoundflag, rv->alen)) ||
                  (compoundbegin && !wordnum && !onlycpdrule && 
                        TESTAFF(rv->astr, compoundbegin, rv->alen)) ||
                  (compoundmiddle && wordnum && !words && !onlycpdrule &&
                    TESTAFF(rv->astr, compoundmiddle, rv->alen)) ||
                  (numdefcpd && onlycpdrule &&
                    ((!words && !wordnum && defcpd_check(&words, wnum, rv, (hentry **) &rwords, 0)) ||
                    (words && defcpd_check(&words, wnum, rv, (hentry **) &rwords, 0))))) ||
                  (scpd != 0 && checkcpdtable[scpd-1].cond != FLAG_NULL &&
                    !TESTAFF(rv->astr, checkcpdtable[scpd-1].cond, rv->alen)))
                  ) {
            rv = rv->next_homonym;
        }

        if (rv) affixed = 0;

        if (!rv) {
            if (onlycpdrule) break;
            if (compoundflag && 
             !(rv = prefix_check(st, i, hu_mov_rule ? IN_CPD_OTHER : IN_CPD_BEGIN, compoundflag))) {
                if (((rv = suffix_check(st, i, 0, NULL, NULL, 0, NULL,
                        FLAG_NULL, compoundflag, hu_mov_rule ? IN_CPD_OTHER : IN_CPD_BEGIN)) || 
                        (compoundmoresuffixes && (rv = suffix_check_twosfx(st, i, 0, NULL, compoundflag)))) && !hu_mov_rule &&
                    sfx->getCont() &&
                        ((compoundforbidflag && TESTAFF(sfx->getCont(), compoundforbidflag, 
                            sfx->getContLen())) || (compoundend &&
                        TESTAFF(sfx->getCont(), compoundend, 
                            sfx->getContLen())))) {
                        rv = NULL;
                }
            }

            if (rv ||
              (((wordnum == 0) && compoundbegin &&
                ((rv = suffix_check(st, i, 0, NULL, NULL, 0, NULL, FLAG_NULL, compoundbegin, hu_mov_rule ? IN_CPD_OTHER : IN_CPD_BEGIN)) ||
                (compoundmoresuffixes && (rv = suffix_check_twosfx(st, i, 0, NULL, compoundbegin))) || 
                (rv = prefix_check(st, i, hu_mov_rule ? IN_CPD_OTHER : IN_CPD_BEGIN, compoundbegin)))) ||
              ((wordnum > 0) && compoundmiddle &&
                ((rv = suffix_check(st, i, 0, NULL, NULL, 0, NULL, FLAG_NULL, compoundmiddle, hu_mov_rule ? IN_CPD_OTHER : IN_CPD_BEGIN)) ||
                (compoundmoresuffixes && (rv = suffix_check_twosfx(st, i, 0, NULL, compoundmiddle))) || 
                (rv = prefix_check(st, i, hu_mov_rule ? IN_CPD_OTHER : IN_CPD_BEGIN, compoundmiddle)))))
              ) checked_prefix = 1;
        
        } else if (rv->astr && (TESTAFF(rv->astr, forbiddenword, rv->alen) ||
            TESTAFF(rv->astr, needaffix, rv->alen) ||
            TESTAFF(rv->astr, ONLYUPCASEFLAG, rv->alen) ||
            (is_sug && nosuggest && TESTAFF(rv->astr, nosuggest, rv->alen))
             )) {
                st[i] = ch;
                
                break;
        }

            
            if ((rv) && !hu_mov_rule &&
                ((pfx && pfx->getCont() &&
                    TESTAFF(pfx->getCont(), compoundforbidflag, 
                        pfx->getContLen())) ||
                (sfx && sfx->getCont() &&
                    TESTAFF(sfx->getCont(), compoundforbidflag, 
                        sfx->getContLen())))) {
                    rv = NULL;
            }

            
            if ((rv) && !checked_prefix && compoundend && !hu_mov_rule &&
                ((pfx && pfx->getCont() &&
                    TESTAFF(pfx->getCont(), compoundend, 
                        pfx->getContLen())) ||
                (sfx && sfx->getCont() &&
                    TESTAFF(sfx->getCont(), compoundend, 
                        sfx->getContLen())))) {
                    rv = NULL;
            }

            
            if ((rv) && !checked_prefix && (wordnum==0) && compoundmiddle && !hu_mov_rule &&
                ((pfx && pfx->getCont() &&
                    TESTAFF(pfx->getCont(), compoundmiddle, 
                        pfx->getContLen())) ||
                (sfx && sfx->getCont() &&
                    TESTAFF(sfx->getCont(), compoundmiddle, 
                        sfx->getContLen())))) {
                    rv = NULL;
            }

        
        if ((rv) && (rv->astr) && (TESTAFF(rv->astr, forbiddenword, rv->alen) ||
            TESTAFF(rv->astr, ONLYUPCASEFLAG, rv->alen) ||
            (is_sug && nosuggest && TESTAFF(rv->astr, nosuggest, rv->alen)))) {
                return NULL;
            }

        
        if ((rv) && compoundroot && 
            (TESTAFF(rv->astr, compoundroot, rv->alen))) {
                wordnum++;
        }

        
        if (((rv) && 
          ( checked_prefix || (words && words[wnum]) ||
            (compoundflag && TESTAFF(rv->astr, compoundflag, rv->alen)) ||
            ((oldwordnum == 0) && compoundbegin && TESTAFF(rv->astr, compoundbegin, rv->alen)) ||
            ((oldwordnum > 0) && compoundmiddle && TESTAFF(rv->astr, compoundmiddle, rv->alen))



            || ((langnum == LANG_hu) && hu_mov_rule && (
                    TESTAFF(rv->astr, 'F', rv->alen) || 
                    TESTAFF(rv->astr, 'G', rv->alen) ||
                    TESTAFF(rv->astr, 'H', rv->alen)
                )
              )

          ) &&
          (
             
             scpd == 0 || checkcpdtable[scpd-1].cond == FLAG_NULL || 
                TESTAFF(rv->astr, checkcpdtable[scpd-1].cond, rv->alen)
          )
          && ! (( checkcompoundtriple && scpd == 0 && !words && 
                   (word[i-1]==word[i]) && (
                      ((i>1) && (word[i-1]==word[i-2])) ||
                      ((word[i-1]==word[i+1])) 
                   )
               ) ||
               (
                 checkcompoundcase && scpd == 0 && !words && cpdcase_check(word, i)
               ))
         )

         || ((!rv) && (langnum == LANG_hu) && hu_mov_rule && (rv = affix_check(st,i)) &&
              (sfx && sfx->getCont() && ( 
                        TESTAFF(sfx->getCont(), (unsigned short) 'x', sfx->getContLen()) ||
                        TESTAFF(sfx->getCont(), (unsigned short) '%', sfx->getContLen())
                    )
               )
             )
         ) { 


            if (langnum == LANG_hu) {
                
                numsyllable += get_syllable(st, i);
                
                if (pfx && (get_syllable(pfx->getKey(),strlen(pfx->getKey())) > 1)) wordnum++;
            }


            
            rv_first = rv;
            st[i] = ch;

        do { 

            
            if (simplifiedtriple) { 
              if (striple) { 
                checkedstriple = 1;
                i--; 
              } else if (i > 2 && *(word+i - 1) == *(word + i - 2)) striple = 1;
            }

            rv = lookup((st+i)); 

        
        while ((rv) && ((needaffix && TESTAFF(rv->astr, needaffix, rv->alen)) ||
                        !((compoundflag && !words && TESTAFF(rv->astr, compoundflag, rv->alen)) ||
                          (compoundend && !words && TESTAFF(rv->astr, compoundend, rv->alen)) ||
                           (numdefcpd && words && defcpd_check(&words, wnum + 1, rv, NULL,1))) ||
                             (scpd != 0 && checkcpdtable[scpd-1].cond2 != FLAG_NULL &&
                                !TESTAFF(rv->astr, checkcpdtable[scpd-1].cond2, rv->alen))
                           )) {
            rv = rv->next_homonym;
        }

            
            if (rv && forceucase && (rv) &&
                (TESTAFF(rv->astr, forceucase, rv->alen)) && !(info && *info & SPELL_ORIGCAP)) rv = NULL;

            if (rv && words && words[wnum + 1]) return rv_first;

            oldnumsyllable2 = numsyllable;
            oldwordnum2 = wordnum;



            if ((rv) && (langnum == LANG_hu) && (TESTAFF(rv->astr, 'I', rv->alen)) && !(TESTAFF(rv->astr, 'J', rv->alen))) {
                numsyllable--;
            }


            
            if ((rv) && (compoundroot) && 
                (TESTAFF(rv->astr, compoundroot, rv->alen))) {
                    wordnum++;
            }

            
            if ((rv) && (rv->astr) && (TESTAFF(rv->astr, forbiddenword, rv->alen) ||
                TESTAFF(rv->astr, ONLYUPCASEFLAG, rv->alen) ||
               (is_sug && nosuggest && TESTAFF(rv->astr, nosuggest, rv->alen)))) return NULL;

            
            
            
            

            if ((rv) && (
                      (compoundflag && TESTAFF(rv->astr, compoundflag, rv->alen)) ||
                      (compoundend && TESTAFF(rv->astr, compoundend, rv->alen))
                    )
                && (
                      ((cpdwordmax==-1) || (wordnum+1<cpdwordmax)) || 
                      ((cpdmaxsyllable!=0) && 
                          (numsyllable + get_syllable(HENTRY_WORD(rv), rv->clen)<=cpdmaxsyllable))
                    ) &&
               (
                 
                 !numcheckcpd || scpd != 0 || !cpdpat_check(word, i, rv_first, rv, 0)
               ) &&
                (
                     (!checkcompounddup || (rv != rv_first))
                   )
            
                && (scpd == 0 || checkcpdtable[scpd-1].cond2 == FLAG_NULL ||
                      TESTAFF(rv->astr, checkcpdtable[scpd-1].cond2, rv->alen))
                )
                 {
                      
                      if (checkcompoundrep && cpdrep_check(word,len)) return NULL;
                      return rv_first;
            }

            numsyllable = oldnumsyllable2;
            wordnum = oldwordnum2;

            
            sfx = NULL;
            sfxflag = FLAG_NULL;
            rv = (compoundflag && !onlycpdrule) ? affix_check((word+i),strlen(word+i), compoundflag, IN_CPD_END) : NULL;
            if (!rv && compoundend && !onlycpdrule) {
                sfx = NULL;
                pfx = NULL;
                rv = affix_check((word+i),strlen(word+i), compoundend, IN_CPD_END);
            }

            if (!rv && numdefcpd && words) {
                rv = affix_check((word+i),strlen(word+i), 0, IN_CPD_END);
                if (rv && defcpd_check(&words, wnum + 1, rv, NULL, 1)) return rv_first;
                rv = NULL;
            }

            
            if (rv && !(scpd == 0 || checkcpdtable[scpd-1].cond2 == FLAG_NULL || 
                TESTAFF(rv->astr, checkcpdtable[scpd-1].cond2, rv->alen))) rv = NULL;

            
            if (rv && numcheckcpd && scpd == 0 && cpdpat_check(word, i, rv_first, rv, affixed)) rv = NULL;

            
            if ((rv) && 
                ((pfx && pfx->getCont() &&
                    TESTAFF(pfx->getCont(), compoundforbidflag, 
                        pfx->getContLen())) ||
                (sfx && sfx->getCont() &&
                    TESTAFF(sfx->getCont(), compoundforbidflag, 
                        sfx->getContLen())))) {
                    rv = NULL;
            }

            
            if (rv && forceucase && (rv) &&
                (TESTAFF(rv->astr, forceucase, rv->alen)) && !(info && *info & SPELL_ORIGCAP)) rv = NULL;

            
            if ((rv) && (rv->astr) && (TESTAFF(rv->astr, forbiddenword, rv->alen) ||
                TESTAFF(rv->astr, ONLYUPCASEFLAG, rv->alen) ||
               (is_sug && nosuggest && TESTAFF(rv->astr, nosuggest, rv->alen)))) return NULL;

            
            
            
            

            if (langnum == LANG_hu) {
                
                numsyllable += get_syllable(word + i, strlen(word + i));

                
                
                if (sfxappnd) {
                    char * tmp = myrevstrdup(sfxappnd);
                    numsyllable -= get_syllable(tmp, strlen(tmp));
                    free(tmp);
                }

                
                if (pfx && (get_syllable(pfx->getKey(),strlen(pfx->getKey())) > 1)) wordnum++;

                
                

                if (cpdsyllablenum) {
                    switch (sfxflag) {
                        case 'c': { numsyllable+=2; break; }
                        case 'J': { numsyllable += 1; break; }
                        case 'I': { if (rv && TESTAFF(rv->astr, 'J', rv->alen)) numsyllable += 1; break; }
                    }
                }
            }

            
            if ((rv) && (compoundroot) && 
                (TESTAFF(rv->astr, compoundroot, rv->alen))) {
                    wordnum++;
            }

            
            
            
            
            if ((rv) && 
                    (
                      ((cpdwordmax == -1) || (wordnum + 1 < cpdwordmax)) || 
                      ((cpdmaxsyllable != 0) && 
                          (numsyllable <= cpdmaxsyllable))
                    )
                && (
                   (!checkcompounddup || (rv != rv_first))
                   )) {
                    
                    if (checkcompoundrep && cpdrep_check(word, len)) return NULL;
                    return rv_first;
            }

            numsyllable = oldnumsyllable2;
            wordnum = oldwordnum2;

            
            if (wordnum < maxwordnum) {
                rv = compound_check((st+i),strlen(st+i), wordnum+1,
                     numsyllable, maxwordnum, wnum + 1, words, 0, is_sug, info);
                
                if (rv && numcheckcpd && ((scpd == 0 && cpdpat_check(word, i, rv_first, rv, affixed)) ||
                   (scpd != 0 && !cpdpat_check(word, i, rv_first, rv, affixed)))) rv = NULL;
            } else {
                rv=NULL;
            }
            if (rv) {
                
                if (checkcompoundrep || forbiddenword) {
                    struct hentry * rv2 = NULL;

                    if (checkcompoundrep && cpdrep_check(word, len)) return NULL;
                    
                    
                    if (strncmp(rv->word, word + i, rv->blen) == 0) {
                        char r = *(st + i + rv->blen);
                        *(st + i + rv->blen) = '\0';
                        
                        if (checkcompoundrep && cpdrep_check(st, i + rv->blen)) {
                            *(st + i + rv->blen) = r;
                            continue;
                        }

                        if (forbiddenword) {
                            rv2 = lookup(word);
                            if (!rv2) rv2 = affix_check(word, len);
                            if (rv2 && rv2->astr && TESTAFF(rv2->astr, forbiddenword, rv2->alen) && 
                                (strncmp(rv2->word, st, i + rv->blen) == 0)) {
                                    return NULL;
                            }
                        }
                        *(st + i + rv->blen) = r;
                    }
                }
                return rv_first;
            }
          } while (striple && !checkedstriple); 

          if (checkedstriple) {
            i++;
            checkedstriple = 0;
            striple = 0;
          }

        } 

        if (soldi != 0) {
          i = soldi;
          soldi = 0;
          len = oldlen;
          cmin = oldcmin;
          cmax = oldcmax;
        }
        scpd++;


        } while (!onlycpdrule && simplifiedcpd && scpd <= numcheckcpd); 

        scpd = 0;
        wordnum = oldwordnum;
        numsyllable = oldnumsyllable;

        if (soldi != 0) {
          i = soldi;
          strcpy(st, word); 
          soldi = 0;
        } else st[i] = ch;

        } while (numdefcpd && oldwordnum == 0 && !onlycpdrule && (onlycpdrule = 1)); 

    }

    return NULL;
}



int AffixMgr::compound_check_morph(const char * word, int len, 
    short wordnum, short numsyllable, short maxwordnum, short wnum, hentry ** words,
    char hu_mov_rule = 0, char ** result = NULL, char * partresult = NULL)
{
    int i;
    short oldnumsyllable, oldnumsyllable2, oldwordnum, oldwordnum2;
    int ok = 0;

    struct hentry * rv = NULL;
    struct hentry * rv_first;
    struct hentry * rwords[MAXWORDLEN]; 
    char st [MAXWORDUTF8LEN + 4];
    char ch;

    int checked_prefix;
    char presult[MAXLNLEN];

    int cmin;
    int cmax;

    int onlycpdrule;
    char affixed = 0;
    hentry ** oldwords = words;

    setcminmax(&cmin, &cmax, word, len);

    strcpy(st, word);

    for (i = cmin; i < cmax; i++) {
        oldnumsyllable = numsyllable;
        oldwordnum = wordnum;
        checked_prefix = 0;

        
        if (utf8) {
            for (; (st[i] & 0xc0) == 0x80; i++);
            if (i >= cmax) return 0;
        }

        words = oldwords;
        onlycpdrule = (words) ? 1 : 0;

        do { 

        oldnumsyllable = numsyllable;
        oldwordnum = wordnum;
        checked_prefix = 0;

        ch = st[i];
        st[i] = '\0';
        sfx = NULL;

        

        affixed = 1;

        *presult = '\0';
        if (partresult) mystrcat(presult, partresult, MAXLNLEN);

        rv = lookup(st); 

        
        while ((rv) && !hu_mov_rule && 
            ((needaffix && TESTAFF(rv->astr, needaffix, rv->alen)) ||
                !((compoundflag && !words && !onlycpdrule && TESTAFF(rv->astr, compoundflag, rv->alen)) ||
                (compoundbegin && !wordnum && !onlycpdrule &&
                        TESTAFF(rv->astr, compoundbegin, rv->alen)) ||
                (compoundmiddle && wordnum && !words && !onlycpdrule &&
                    TESTAFF(rv->astr, compoundmiddle, rv->alen)) ||
                  (numdefcpd && onlycpdrule &&
                    ((!words && !wordnum && defcpd_check(&words, wnum, rv, (hentry **) &rwords, 0)) ||
                    (words && defcpd_check(&words, wnum, rv, (hentry **) &rwords, 0))))
                  ))) {
            rv = rv->next_homonym;
        }

        if (rv) affixed = 0;

        if (rv)  {
            sprintf(presult + strlen(presult), "%c%s%s", MSEP_FLD, MORPH_PART, st);
            if (!HENTRY_FIND(rv, MORPH_STEM)) {
                sprintf(presult + strlen(presult), "%c%s%s", MSEP_FLD, MORPH_STEM, st);
            }
            

            if (HENTRY_DATA(rv)) {
                sprintf(presult + strlen(presult), "%c%s", MSEP_FLD, HENTRY_DATA2(rv));
            }
        }        

        if (!rv) {
            if (onlycpdrule && strlen(*result) > MAXLNLEN/10) break;
            if (compoundflag &&
             !(rv = prefix_check(st, i, hu_mov_rule ? IN_CPD_OTHER : IN_CPD_BEGIN, compoundflag))) {
                if (((rv = suffix_check(st, i, 0, NULL, NULL, 0, NULL,
                        FLAG_NULL, compoundflag, hu_mov_rule ? IN_CPD_OTHER : IN_CPD_BEGIN)) ||
                        (compoundmoresuffixes && (rv = suffix_check_twosfx(st, i, 0, NULL, compoundflag)))) && !hu_mov_rule &&
                    sfx->getCont() &&
                        ((compoundforbidflag && TESTAFF(sfx->getCont(), compoundforbidflag, 
                            sfx->getContLen())) || (compoundend &&
                        TESTAFF(sfx->getCont(), compoundend, 
                            sfx->getContLen())))) {
                        rv = NULL;
                }
            }

            if (rv ||
              (((wordnum == 0) && compoundbegin &&
                ((rv = suffix_check(st, i, 0, NULL, NULL, 0, NULL, FLAG_NULL, compoundbegin, hu_mov_rule ? IN_CPD_OTHER : IN_CPD_BEGIN)) ||
                (compoundmoresuffixes && (rv = suffix_check_twosfx(st, i, 0, NULL, compoundbegin))) ||  
                (rv = prefix_check(st, i, hu_mov_rule ? IN_CPD_OTHER : IN_CPD_BEGIN, compoundbegin)))) ||
              ((wordnum > 0) && compoundmiddle &&
                ((rv = suffix_check(st, i, 0, NULL, NULL, 0, NULL, FLAG_NULL, compoundmiddle, hu_mov_rule ? IN_CPD_OTHER : IN_CPD_BEGIN)) ||
                (compoundmoresuffixes && (rv = suffix_check_twosfx(st, i, 0, NULL, compoundmiddle))) ||  
                (rv = prefix_check(st, i, hu_mov_rule ? IN_CPD_OTHER : IN_CPD_BEGIN, compoundmiddle)))))
              ) {
                
                char * p = NULL;
                if (compoundflag) p = affix_check_morph(st, i, compoundflag);
                if (!p || (*p == '\0')) {
                   if (p) free(p);
                   p = NULL;
                   if ((wordnum == 0) && compoundbegin) {
                     p = affix_check_morph(st, i, compoundbegin);
                   } else if ((wordnum > 0) && compoundmiddle) {
                     p = affix_check_morph(st, i, compoundmiddle);                   
                   }
                }
                if (p && (*p != '\0')) {
                    sprintf(presult + strlen(presult), "%c%s%s%s", MSEP_FLD,
                        MORPH_PART, st, line_uniq_app(&p, MSEP_REC));
                }
                if (p) free(p);
                checked_prefix = 1;
            }
        
        } else if (rv->astr && (TESTAFF(rv->astr, forbiddenword, rv->alen) ||
            TESTAFF(rv->astr, ONLYUPCASEFLAG, rv->alen) ||
            TESTAFF(rv->astr, needaffix, rv->alen))) {
                st[i] = ch;
                continue;
        }

            
            if ((rv) && !hu_mov_rule &&
                ((pfx && pfx->getCont() &&
                    TESTAFF(pfx->getCont(), compoundforbidflag, 
                        pfx->getContLen())) ||
                (sfx && sfx->getCont() &&
                    TESTAFF(sfx->getCont(), compoundforbidflag, 
                        sfx->getContLen())))) {
                    continue;
            }

            
            if ((rv) && !checked_prefix && compoundend && !hu_mov_rule &&
                ((pfx && pfx->getCont() &&
                    TESTAFF(pfx->getCont(), compoundend, 
                        pfx->getContLen())) ||
                (sfx && sfx->getCont() &&
                    TESTAFF(sfx->getCont(), compoundend, 
                        sfx->getContLen())))) {
                    continue;
            }

            
            if ((rv) && !checked_prefix && (wordnum==0) && compoundmiddle && !hu_mov_rule &&
                ((pfx && pfx->getCont() &&
                    TESTAFF(pfx->getCont(), compoundmiddle, 
                        pfx->getContLen())) ||
                (sfx && sfx->getCont() &&
                    TESTAFF(sfx->getCont(), compoundmiddle, 
                        sfx->getContLen())))) {
                    rv = NULL;
            }       

        
        if ((rv) && (rv->astr) && (TESTAFF(rv->astr, forbiddenword, rv->alen)
            || TESTAFF(rv->astr, ONLYUPCASEFLAG, rv->alen))) continue;

        
        if ((rv) && (compoundroot) && 
            (TESTAFF(rv->astr, compoundroot, rv->alen))) {
                wordnum++;
        }

        
        if (((rv) && 
          ( checked_prefix || (words && words[wnum]) ||
            (compoundflag && TESTAFF(rv->astr, compoundflag, rv->alen)) ||
            ((oldwordnum == 0) && compoundbegin && TESTAFF(rv->astr, compoundbegin, rv->alen)) ||
            ((oldwordnum > 0) && compoundmiddle && TESTAFF(rv->astr, compoundmiddle, rv->alen)) 

            || ((langnum == LANG_hu) && 
                hu_mov_rule && (
                    TESTAFF(rv->astr, 'F', rv->alen) ||
                    TESTAFF(rv->astr, 'G', rv->alen) ||
                    TESTAFF(rv->astr, 'H', rv->alen)
                )
              )

          )
          && ! (( checkcompoundtriple && !words && 
                   (word[i-1]==word[i]) && (
                      ((i>1) && (word[i-1]==word[i-2])) || 
                      ((word[i-1]==word[i+1])) 
                   )
               ) ||
               (
                   
                   numcheckcpd && !words && cpdpat_check(word, i, rv, NULL, affixed)
               ) ||
               ( 
                 checkcompoundcase && !words && cpdcase_check(word, i)
               ))
         )

         || ((!rv) && (langnum == LANG_hu) && hu_mov_rule && (rv = affix_check(st,i)) &&
              (sfx && sfx->getCont() && (
                        TESTAFF(sfx->getCont(), (unsigned short) 'x', sfx->getContLen()) ||
                        TESTAFF(sfx->getCont(), (unsigned short) '%', sfx->getContLen())
                    )                
               )
             )

         ) {


            if (langnum == LANG_hu) {
                
                numsyllable += get_syllable(st, i);

                
                if (pfx && (get_syllable(pfx->getKey(),strlen(pfx->getKey())) > 1)) wordnum++;
            }


            
            rv_first = rv;
            rv = lookup((word+i)); 

        
        while ((rv) && ((needaffix && TESTAFF(rv->astr, needaffix, rv->alen)) ||
                        !((compoundflag && !words && TESTAFF(rv->astr, compoundflag, rv->alen)) ||
                          (compoundend && !words && TESTAFF(rv->astr, compoundend, rv->alen)) ||
                           (numdefcpd && words && defcpd_check(&words, wnum + 1, rv, NULL,1))))) {
            rv = rv->next_homonym;
        }

            if (rv && words && words[wnum + 1]) {
                  mystrcat(*result, presult, MAXLNLEN);
                  mystrcat(*result, " ", MAXLNLEN);
                  mystrcat(*result, MORPH_PART, MAXLNLEN);
                  mystrcat(*result, word+i, MAXLNLEN);
                  if (complexprefixes && HENTRY_DATA(rv)) mystrcat(*result, HENTRY_DATA2(rv), MAXLNLEN);
                  if (!HENTRY_FIND(rv, MORPH_STEM)) {
                    mystrcat(*result, " ", MAXLNLEN);
                    mystrcat(*result, MORPH_STEM, MAXLNLEN);
                    mystrcat(*result, HENTRY_WORD(rv), MAXLNLEN);
                  }
                  

                  if (!complexprefixes && HENTRY_DATA(rv)) {
                    mystrcat(*result, " ", MAXLNLEN);
                    mystrcat(*result, HENTRY_DATA2(rv), MAXLNLEN);
                  }
                  mystrcat(*result, "\n", MAXLNLEN);
                  ok = 1;
                  return 0;
            }

            oldnumsyllable2 = numsyllable;
            oldwordnum2 = wordnum;


            if ((rv) && (langnum == LANG_hu) && (TESTAFF(rv->astr, 'I', rv->alen)) && !(TESTAFF(rv->astr, 'J', rv->alen))) {
                numsyllable--;
            }

            
            if ((rv) && (compoundroot) && 
                (TESTAFF(rv->astr, compoundroot, rv->alen))) {
                    wordnum++;
            }

            
            if ((rv) && (rv->astr) && (TESTAFF(rv->astr, forbiddenword, rv->alen) ||
                TESTAFF(rv->astr, ONLYUPCASEFLAG, rv->alen))) {
                st[i] = ch;
                continue;
            }

            
            
            
            
            if ((rv) && (
                      (compoundflag && TESTAFF(rv->astr, compoundflag, rv->alen)) ||
                      (compoundend && TESTAFF(rv->astr, compoundend, rv->alen))
                    )
                && (
                      ((cpdwordmax==-1) || (wordnum+1<cpdwordmax)) || 
                      ((cpdmaxsyllable!=0) &&
                          (numsyllable+get_syllable(HENTRY_WORD(rv),rv->blen)<=cpdmaxsyllable))
                    )
                && (
                     (!checkcompounddup || (rv != rv_first))
                   )
                )
                 {
                      
                      mystrcat(*result, presult, MAXLNLEN);
                      mystrcat(*result, " ", MAXLNLEN);
                      mystrcat(*result, MORPH_PART, MAXLNLEN);
                      mystrcat(*result, word+i, MAXLNLEN);

                      if (HENTRY_DATA(rv)) {
                        if (complexprefixes) mystrcat(*result, HENTRY_DATA2(rv), MAXLNLEN);
                        if (! HENTRY_FIND(rv, MORPH_STEM)) {
                           mystrcat(*result, " ", MAXLNLEN);
                           mystrcat(*result, MORPH_STEM, MAXLNLEN);
                           mystrcat(*result, HENTRY_WORD(rv), MAXLNLEN);
                        }
                        

                        if (!complexprefixes) {
                            mystrcat(*result, " ", MAXLNLEN);
                            mystrcat(*result, HENTRY_DATA2(rv), MAXLNLEN);
                        }
                      }
                      mystrcat(*result, "\n", MAXLNLEN);
                              ok = 1;
            }

            numsyllable = oldnumsyllable2 ;
            wordnum = oldwordnum2;

            
            sfx = NULL;
            sfxflag = FLAG_NULL;

            if (compoundflag && !onlycpdrule) rv = affix_check((word+i),strlen(word+i), compoundflag); else rv = NULL;

            if (!rv && compoundend && !onlycpdrule) {
                sfx = NULL;
                pfx = NULL;
                rv = affix_check((word+i),strlen(word+i), compoundend);
            }

            if (!rv && numdefcpd && words) {
                rv = affix_check((word+i),strlen(word+i), 0, IN_CPD_END);
                if (rv && words && defcpd_check(&words, wnum + 1, rv, NULL, 1)) {
                      char * m = NULL;
                      if (compoundflag) m = affix_check_morph((word+i),strlen(word+i), compoundflag);
                      if ((!m || *m == '\0') && compoundend) {
                            if (m) free(m);
                            m = affix_check_morph((word+i),strlen(word+i), compoundend);
                      }
                      mystrcat(*result, presult, MAXLNLEN);
                      if (m || (*m != '\0')) {
                        sprintf(*result + strlen(*result), "%c%s%s%s", MSEP_FLD,
                            MORPH_PART, word + i, line_uniq_app(&m, MSEP_REC));
                      }
                      if (m) free(m);
                      mystrcat(*result, "\n", MAXLNLEN);
                      ok = 1;
                }
            }

            
            if ((rv) && 
                ((pfx && pfx->getCont() &&
                    TESTAFF(pfx->getCont(), compoundforbidflag, 
                        pfx->getContLen())) ||
                (sfx && sfx->getCont() &&
                    TESTAFF(sfx->getCont(), compoundforbidflag, 
                        sfx->getContLen())))) {
                    rv = NULL;
            }

            
            if ((rv) && (rv->astr) && (TESTAFF(rv->astr,forbiddenword,rv->alen) ||
                    TESTAFF(rv->astr, ONLYUPCASEFLAG, rv->alen))
                    && (! TESTAFF(rv->astr, needaffix, rv->alen))) {
                        st[i] = ch;
                        continue;
                    }

            if (langnum == LANG_hu) {
                
                numsyllable += get_syllable(word + i, strlen(word + i));

                
                
                if (sfxappnd) {
                    char * tmp = myrevstrdup(sfxappnd);
                    numsyllable -= get_syllable(tmp, strlen(tmp));
                    free(tmp);
                }

                
                if (pfx && (get_syllable(pfx->getKey(),strlen(pfx->getKey())) > 1)) wordnum++;

                
                

                if (cpdsyllablenum) {
                    switch (sfxflag) {
                        case 'c': { numsyllable+=2; break; }
                        case 'J': { numsyllable += 1; break; }
                        case 'I': { if (rv && TESTAFF(rv->astr, 'J', rv->alen)) numsyllable += 1; break; }
                    }
                }
            }

            
            if ((rv) && (compoundroot) && 
                (TESTAFF(rv->astr, compoundroot, rv->alen))) {
                    wordnum++;
            }
            
            
            
            
            if ((rv) && 
                    (
                      ((cpdwordmax==-1) || (wordnum+1<cpdwordmax)) || 
                      ((cpdmaxsyllable!=0) &&
                          (numsyllable <= cpdmaxsyllable))
                    )
                && (
                   (!checkcompounddup || (rv != rv_first))
                   )) {
                      char * m = NULL;
                      if (compoundflag) m = affix_check_morph((word+i),strlen(word+i), compoundflag);
                      if ((!m || *m == '\0') && compoundend) {
                            if (m) free(m);
                            m = affix_check_morph((word+i),strlen(word+i), compoundend);
                      }
                      mystrcat(*result, presult, MAXLNLEN);
                      if (m && (*m != '\0')) {
                        sprintf(*result + strlen(*result), "%c%s%s%s", MSEP_FLD,
                            MORPH_PART, word + i, line_uniq_app(&m, MSEP_REC));
                      }
                      if (m) free(m);
                      sprintf(*result + strlen(*result), "%c", MSEP_REC);
                      ok = 1;
            }

            numsyllable = oldnumsyllable2;
            wordnum = oldwordnum2;

            
            if ((wordnum < maxwordnum) && (ok == 0)) {
                        compound_check_morph((word+i),strlen(word+i), wordnum+1, 
                             numsyllable, maxwordnum, wnum + 1, words, 0, result, presult);
            } else {
                rv=NULL;
            }
        }
        st[i] = ch;
        wordnum = oldwordnum;
        numsyllable = oldnumsyllable;

        } while (numdefcpd && oldwordnum == 0 && !onlycpdrule && (onlycpdrule = 1)); 

    }
    return 0;
}    

 











inline int AffixMgr::isRevSubset(const char * s1, const char * end_of_s2, int len)
 {
    while ((len > 0) && (*s1 != '\0') && ((*s1 == *end_of_s2) || (*s1 == '.'))) {
        s1++;
        end_of_s2--;
        len--;
    }
    return (*s1 == '\0');
 }



struct hentry * AffixMgr::suffix_check (const char * word, int len, 
       int sfxopts, PfxEntry * ppfx, char ** wlst, int maxSug, int * ns, 
       const FLAG cclass, const FLAG needflag, char in_compound)
{
    struct hentry * rv = NULL;
    PfxEntry* ep = ppfx;

    
    SfxEntry * se = sStart[0];

    while (se) {
        if (!cclass || se->getCont()) {
            
            if ((((in_compound != IN_CPD_BEGIN)) || 
             
             (se->getCont() && compoundpermitflag &&
                TESTAFF(se->getCont(),compoundpermitflag,se->getContLen()))) && (!circumfix ||
              
              ((!ppfx || !(ep->getCont()) || !TESTAFF(ep->getCont(),
                   circumfix, ep->getContLen())) &&
               (!se->getCont() || !(TESTAFF(se->getCont(),circumfix,se->getContLen())))) ||
              
              ((ppfx && (ep->getCont()) && TESTAFF(ep->getCont(),
                   circumfix, ep->getContLen())) &&
               (se->getCont() && (TESTAFF(se->getCont(),circumfix,se->getContLen())))))  &&
            
              (in_compound || 
                 !(se->getCont() && (TESTAFF(se->getCont(), onlyincompound, se->getContLen())))) &&
            
              (cclass || 
                   !(se->getCont() && TESTAFF(se->getCont(), needaffix, se->getContLen())) ||
                   (ppfx && !((ep->getCont()) &&
                     TESTAFF(ep->getCont(), needaffix,
                       ep->getContLen())))
              )) {
                rv = se->checkword(word,len, sfxopts, ppfx, wlst, maxSug, ns, (FLAG) cclass, 
                    needflag, (in_compound ? 0 : onlyincompound));
                if (rv) {
                    sfx=se; 
                    return rv;
                }
            }
        }
       se = se->getNext();
    }

    
    if (len == 0) return NULL; 
    unsigned char sp= *((const unsigned char *)(word + len - 1));
    SfxEntry * sptr = sStart[sp];

    while (sptr) {
        if (isRevSubset(sptr->getKey(), word + len - 1, len)
        ) {
            
            if ((((in_compound != IN_CPD_BEGIN)) || 
             
             (sptr->getCont() && compoundpermitflag &&
                TESTAFF(sptr->getCont(),compoundpermitflag,sptr->getContLen()))) && (!circumfix ||
              
              ((!ppfx || !(ep->getCont()) || !TESTAFF(ep->getCont(),
                   circumfix, ep->getContLen())) &&
               (!sptr->getCont() || !(TESTAFF(sptr->getCont(),circumfix,sptr->getContLen())))) ||
              
              ((ppfx && (ep->getCont()) && TESTAFF(ep->getCont(),
                   circumfix, ep->getContLen())) &&
               (sptr->getCont() && (TESTAFF(sptr->getCont(),circumfix,sptr->getContLen())))))  &&
            
              (in_compound || 
                 !((sptr->getCont() && (TESTAFF(sptr->getCont(), onlyincompound, sptr->getContLen()))))) &&
            
              (cclass || 
                  !(sptr->getCont() && TESTAFF(sptr->getCont(), needaffix, sptr->getContLen())) ||
                  (ppfx && !((ep->getCont()) &&
                     TESTAFF(ep->getCont(), needaffix,
                       ep->getContLen())))
              )
            ) if (in_compound != IN_CPD_END || ppfx || !(sptr->getCont() && TESTAFF(sptr->getCont(), onlyincompound, sptr->getContLen()))) {
                rv = sptr->checkword(word,len, sfxopts, ppfx, wlst,
                    maxSug, ns, cclass, needflag, (in_compound ? 0 : onlyincompound));
                if (rv) {
                    sfx=sptr; 
                    sfxflag = sptr->getFlag(); 
                    if (!sptr->getCont()) sfxappnd=sptr->getKey(); 
                    return rv;
                }
             }
             sptr = sptr->getNextEQ();
        } else {
             sptr = sptr->getNextNE();
        }
    }

    return NULL;
}



struct hentry * AffixMgr::suffix_check_twosfx(const char * word, int len, 
       int sfxopts, PfxEntry * ppfx, const FLAG needflag)
{
    struct hentry * rv = NULL;

    
    SfxEntry * se = sStart[0];
    while (se) {
        if (contclasses[se->getFlag()])
        {
            rv = se->check_twosfx(word,len, sfxopts, ppfx, needflag);
            if (rv) return rv;
        }
        se = se->getNext();
    }

    
    if (len == 0) return NULL; 
    unsigned char sp = *((const unsigned char *)(word + len - 1));
    SfxEntry * sptr = sStart[sp];

    while (sptr) {
        if (isRevSubset(sptr->getKey(), word + len - 1, len)) {
            if (contclasses[sptr->getFlag()])
            {
                rv = sptr->check_twosfx(word,len, sfxopts, ppfx, needflag);
                if (rv) {
                    sfxflag = sptr->getFlag(); 
                    if (!sptr->getCont()) sfxappnd=sptr->getKey(); 
                    return rv;
                }
            }
            sptr = sptr->getNextEQ();
        } else {
             sptr = sptr->getNextNE();
        }
    }

    return NULL;
}

char * AffixMgr::suffix_check_twosfx_morph(const char * word, int len, 
       int sfxopts, PfxEntry * ppfx, const FLAG needflag)
{
    char result[MAXLNLEN];
    char result2[MAXLNLEN];
    char result3[MAXLNLEN];
    
    char * st;

    result[0] = '\0';
    result2[0] = '\0';
    result3[0] = '\0';

    
    SfxEntry * se = sStart[0];
    while (se) {
        if (contclasses[se->getFlag()])
        {
            st = se->check_twosfx_morph(word,len, sfxopts, ppfx, needflag);
            if (st) {
                if (ppfx) {
                    if (ppfx->getMorph()) {
                        mystrcat(result, ppfx->getMorph(), MAXLNLEN);
                        mystrcat(result, " ", MAXLNLEN);
                    } else debugflag(result, ppfx->getFlag());
                }
                mystrcat(result, st, MAXLNLEN);
                free(st);
                if (se->getMorph()) {
                    mystrcat(result, " ", MAXLNLEN);
                    mystrcat(result, se->getMorph(), MAXLNLEN);
                } else debugflag(result, se->getFlag());
                mystrcat(result, "\n", MAXLNLEN);
            }
        }
        se = se->getNext();
    }

    
    if (len == 0) return NULL; 
    unsigned char sp = *((const unsigned char *)(word + len - 1));
    SfxEntry * sptr = sStart[sp];

    while (sptr) {
        if (isRevSubset(sptr->getKey(), word + len - 1, len)) {
            if (contclasses[sptr->getFlag()]) 
            {
                st = sptr->check_twosfx_morph(word,len, sfxopts, ppfx, needflag);
                if (st) {
                    sfxflag = sptr->getFlag(); 
                    if (!sptr->getCont()) sfxappnd=sptr->getKey(); 
                    strcpy(result2, st);
                    free(st);

                result3[0] = '\0';

                if (sptr->getMorph()) {
                    mystrcat(result3, " ", MAXLNLEN);
                    mystrcat(result3, sptr->getMorph(), MAXLNLEN);
                } else debugflag(result3, sptr->getFlag());
                strlinecat(result2, result3);
                mystrcat(result2, "\n", MAXLNLEN);
                mystrcat(result,  result2, MAXLNLEN);
                }
            }
            sptr = sptr->getNextEQ();
        } else {
             sptr = sptr->getNextNE();
        }
    }
    if (*result) return mystrdup(result);
    return NULL;
}

char * AffixMgr::suffix_check_morph(const char * word, int len, 
       int sfxopts, PfxEntry * ppfx, const FLAG cclass, const FLAG needflag, char in_compound)
{
    char result[MAXLNLEN];
    
    struct hentry * rv = NULL;

    result[0] = '\0';

    PfxEntry* ep = ppfx;

    
    SfxEntry * se = sStart[0];
    while (se) {
        if (!cclass || se->getCont()) {
            
            if (((((in_compound != IN_CPD_BEGIN)) || 
             
             (se->getCont() && compoundpermitflag &&
                TESTAFF(se->getCont(),compoundpermitflag,se->getContLen()))) && (!circumfix ||
              
              ((!ppfx || !(ep->getCont()) || !TESTAFF(ep->getCont(),
                   circumfix, ep->getContLen())) &&
               (!se->getCont() || !(TESTAFF(se->getCont(),circumfix,se->getContLen())))) ||
              
              ((ppfx && (ep->getCont()) && TESTAFF(ep->getCont(),
                   circumfix, ep->getContLen())) &&
               (se->getCont() && (TESTAFF(se->getCont(),circumfix,se->getContLen())))))  &&
            
              (in_compound || 
                 !((se->getCont() && (TESTAFF(se->getCont(), onlyincompound, se->getContLen()))))) &&
            
              (cclass || 
                   !(se->getCont() && TESTAFF(se->getCont(), needaffix, se->getContLen())) ||
                   (ppfx && !((ep->getCont()) &&
                     TESTAFF(ep->getCont(), needaffix,
                       ep->getContLen())))
              )
            ))
            rv = se->checkword(word, len, sfxopts, ppfx, NULL, 0, 0, cclass, needflag);
         while (rv) {
           if (ppfx) {
                if (ppfx->getMorph()) {
                    mystrcat(result, ppfx->getMorph(), MAXLNLEN);
                    mystrcat(result, " ", MAXLNLEN);
                } else debugflag(result, ppfx->getFlag());
            }
            if (complexprefixes && HENTRY_DATA(rv)) mystrcat(result, HENTRY_DATA2(rv), MAXLNLEN);
            if (! HENTRY_FIND(rv, MORPH_STEM)) {
                mystrcat(result, " ", MAXLNLEN);                                
                mystrcat(result, MORPH_STEM, MAXLNLEN);
                mystrcat(result, HENTRY_WORD(rv), MAXLNLEN);
            }
            

            
            if (!complexprefixes && HENTRY_DATA(rv)) {
                    mystrcat(result, " ", MAXLNLEN);                                
                    mystrcat(result, HENTRY_DATA2(rv), MAXLNLEN);
            }
            if (se->getMorph()) {
                mystrcat(result, " ", MAXLNLEN);                                
                mystrcat(result, se->getMorph(), MAXLNLEN);
            } else debugflag(result, se->getFlag());
            mystrcat(result, "\n", MAXLNLEN);
            rv = se->get_next_homonym(rv, sfxopts, ppfx, cclass, needflag);
         }
       }
       se = se->getNext();
    }

    
    if (len == 0) return NULL; 
    unsigned char sp = *((const unsigned char *)(word + len - 1));
    SfxEntry * sptr = sStart[sp];

    while (sptr) {
        if (isRevSubset(sptr->getKey(), word + len - 1, len)
        ) {
            
            if (((((in_compound != IN_CPD_BEGIN)) || 
             
             (sptr->getCont() && compoundpermitflag &&
                TESTAFF(sptr->getCont(),compoundpermitflag,sptr->getContLen()))) && (!circumfix ||
              
              ((!ppfx || !(ep->getCont()) || !TESTAFF(ep->getCont(),
                   circumfix, ep->getContLen())) &&
               (!sptr->getCont() || !(TESTAFF(sptr->getCont(),circumfix,sptr->getContLen())))) ||
              
              ((ppfx && (ep->getCont()) && TESTAFF(ep->getCont(),
                   circumfix, ep->getContLen())) &&
               (sptr->getCont() && (TESTAFF(sptr->getCont(),circumfix,sptr->getContLen())))))  &&
            
              (in_compound || 
                 !((sptr->getCont() && (TESTAFF(sptr->getCont(), onlyincompound, sptr->getContLen()))))) &&
            
              (cclass || !(sptr->getCont() && 
                   TESTAFF(sptr->getCont(), needaffix, sptr->getContLen())))
            )) rv = sptr->checkword(word,len, sfxopts, ppfx, NULL, 0, 0, cclass, needflag);
            while (rv) {
                    if (ppfx) {
                        if (ppfx->getMorph()) {
                            mystrcat(result, ppfx->getMorph(), MAXLNLEN);
                            mystrcat(result, " ", MAXLNLEN);
                        } else debugflag(result, ppfx->getFlag());
                    }    
                    if (complexprefixes && HENTRY_DATA(rv)) mystrcat(result, HENTRY_DATA2(rv), MAXLNLEN);
                    if (! HENTRY_FIND(rv, MORPH_STEM)) {
                            mystrcat(result, " ", MAXLNLEN);                                
                            mystrcat(result, MORPH_STEM, MAXLNLEN);
                            mystrcat(result, HENTRY_WORD(rv), MAXLNLEN);
                    }
                    


                    if (!complexprefixes && HENTRY_DATA(rv)) {
                        mystrcat(result, " ", MAXLNLEN);                                
                        mystrcat(result, HENTRY_DATA2(rv), MAXLNLEN);
                    }

                if (sptr->getMorph()) {
                    mystrcat(result, " ", MAXLNLEN);
                    mystrcat(result, sptr->getMorph(), MAXLNLEN);
                } else debugflag(result, sptr->getFlag());
                mystrcat(result, "\n", MAXLNLEN);
                rv = sptr->get_next_homonym(rv, sfxopts, ppfx, cclass, needflag);
            }
             sptr = sptr->getNextEQ();
        } else {
             sptr = sptr->getNextNE();
        }
    }

    if (*result) return mystrdup(result);
    return NULL;
}


struct hentry * AffixMgr::affix_check (const char * word, int len, const FLAG needflag, char in_compound)
{
    struct hentry * rv= NULL;

    
    rv = prefix_check(word, len, in_compound, needflag);
    if (rv) return rv;

    
    rv = suffix_check(word, len, 0, NULL, NULL, 0, NULL, FLAG_NULL, needflag, in_compound);

    if (havecontclass) {
        sfx = NULL;
        pfx = NULL;

        if (rv) return rv;
        
        rv = suffix_check_twosfx(word, len, 0, NULL, needflag);

        if (rv) return rv;
        
        rv = prefix_check_twosfx(word, len, IN_CPD_NOT, needflag);
    }

    return rv;
}


char * AffixMgr::affix_check_morph(const char * word, int len, const FLAG needflag, char in_compound)
{
    char result[MAXLNLEN];
    char * st = NULL;

    *result = '\0';
    
    
    st = prefix_check_morph(word, len, in_compound);
    if (st) {
        mystrcat(result, st, MAXLNLEN);
        free(st);
    }

    
    st = suffix_check_morph(word, len, 0, NULL, '\0', needflag, in_compound);
    if (st) {
        mystrcat(result, st, MAXLNLEN);
        free(st);
    }

    if (havecontclass) {
        sfx = NULL;
        pfx = NULL;
        
        st = suffix_check_twosfx_morph(word, len, 0, NULL, needflag);
        if (st) {
            mystrcat(result, st, MAXLNLEN);
            free(st);
        }

        
        st = prefix_check_twosfx_morph(word, len, IN_CPD_NOT, needflag);
        if (st) {
            mystrcat(result, st, MAXLNLEN);
            free(st);
        }
    }

    return mystrdup(result);
}

char * AffixMgr::morphgen(char * ts, int wl, const unsigned short * ap,
    unsigned short al, char * morph, char * targetmorph, int level)
{
    
    char * stemmorph;
    char * stemmorphcatpos;
    char mymorph[MAXLNLEN];

    if (!morph) return NULL;

    
    if (TESTAFF(ap, substandard, al)) return NULL;

    if (morphcmp(morph, targetmorph) == 0) return mystrdup(ts);



    
    if (strstr(morph, MORPH_INFL_SFX) || strstr(morph, MORPH_DERI_SFX)) {
        stemmorph = mymorph;
        strcpy(stemmorph, morph);
        mystrcat(stemmorph, " ", MAXLNLEN);
        stemmorphcatpos = stemmorph + strlen(stemmorph);
    } else {
        stemmorph = morph;
        stemmorphcatpos = NULL;
    }

    for (int i = 0; i < al; i++) {
        const unsigned char c = (unsigned char) (ap[i] & 0x00FF);
        SfxEntry * sptr = sFlag[c];
        while (sptr) {
            if (sptr->getFlag() == ap[i] && sptr->getMorph() && ((sptr->getContLen() == 0) || 
                
                !TESTAFF(sptr->getCont(), substandard, sptr->getContLen()))) {

                if (stemmorphcatpos) strcpy(stemmorphcatpos, sptr->getMorph());
                else stemmorph = (char *) sptr->getMorph();

                int cmp = morphcmp(stemmorph, targetmorph);

                if (cmp == 0) {
                    char * newword = sptr->add(ts, wl);
                    if (newword) {
                        hentry * check = pHMgr->lookup(newword); 
                        if (!check || !check->astr || 
                            !(TESTAFF(check->astr, forbiddenword, check->alen) || 
                              TESTAFF(check->astr, ONLYUPCASEFLAG, check->alen))) {
                                return newword;
                        }
                        free(newword);
                    }
                }
                
                
                if ((level == 0) && (cmp == 1) && (sptr->getContLen() > 0) &&

                    !TESTAFF(sptr->getCont(), substandard, sptr->getContLen())) {
                    char * newword = sptr->add(ts, wl);
                    if (newword) {
                        char * newword2 = morphgen(newword, strlen(newword), sptr->getCont(),
                            sptr->getContLen(), stemmorph, targetmorph, 1);

                        if (newword2) {
                            free(newword);
                            return newword2;
                        }
                        free(newword);
                        newword = NULL;
                    }
                }
            }
            sptr = sptr->getFlgNxt();
        }
    }
   return NULL;
}


int AffixMgr::expand_rootword(struct guessword * wlst, int maxn, const char * ts,
    int wl, const unsigned short * ap, unsigned short al, char * bad, int badl,
    char * phon)
{
    int nh=0;
    
    if ((nh < maxn) && !(al && ((needaffix && TESTAFF(ap, needaffix, al)) ||
         (onlyincompound && TESTAFF(ap, onlyincompound, al))))) {
       wlst[nh].word = mystrdup(ts);
       if (!wlst[nh].word) return 0;
       wlst[nh].allow = (1 == 0);
       wlst[nh].orig = NULL;
       nh++;
       
       if (phon && (nh < maxn)) {
    	    wlst[nh].word = mystrdup(phon);
            if (!wlst[nh].word) return nh - 1;
    	    wlst[nh].allow = (1 == 0);
    	    wlst[nh].orig = mystrdup(ts);
            if (!wlst[nh].orig) return nh - 1;
    	    nh++;
       }
    }

    
    for (int i = 0; i < al; i++) {
       const unsigned char c = (unsigned char) (ap[i] & 0x00FF);
       SfxEntry * sptr = sFlag[c];
       while (sptr) {
         if ((sptr->getFlag() == ap[i]) && (!sptr->getKeyLen() || ((badl > sptr->getKeyLen()) &&
                (strcmp(sptr->getAffix(), bad + badl - sptr->getKeyLen()) == 0))) &&
                
                !(sptr->getCont() && ((needaffix && 
                      TESTAFF(sptr->getCont(), needaffix, sptr->getContLen())) ||
                  (circumfix && 
                      TESTAFF(sptr->getCont(), circumfix, sptr->getContLen())) ||
                  (onlyincompound && 
                      TESTAFF(sptr->getCont(), onlyincompound, sptr->getContLen()))))
                ) {
            char * newword = sptr->add(ts, wl);
            if (newword) {
                if (nh < maxn) {
                    wlst[nh].word = newword;
                    wlst[nh].allow = sptr->allowCross();
                    wlst[nh].orig = NULL;
                    nh++;
                    
    		    if (phon && (nh < maxn)) {
    			char st[MAXWORDUTF8LEN];
    			strcpy(st, phon);
    			strcat(st, sptr->getKey());
    			reverseword(st + strlen(phon));
    			wlst[nh].word = mystrdup(st);
    			if (!wlst[nh].word) return nh - 1;
    			wlst[nh].allow = (1 == 0);
    			wlst[nh].orig = mystrdup(newword);
                        if (!wlst[nh].orig) return nh - 1;
    			nh++;
    		    }
                } else {
                    free(newword);
                }
            }
         }
         sptr = sptr->getFlgNxt();
       }
    }

    int n = nh;

    
    for (int j=1;j<n ;j++)
       if (wlst[j].allow) {
          for (int k = 0; k < al; k++) {
             const unsigned char c = (unsigned char) (ap[k] & 0x00FF);
             PfxEntry * cptr = pFlag[c];
             while (cptr) {
                if ((cptr->getFlag() == ap[k]) && cptr->allowCross() && (!cptr->getKeyLen() || ((badl > cptr->getKeyLen()) &&
                        (strncmp(cptr->getKey(), bad, cptr->getKeyLen()) == 0)))) {
                    int l1 = strlen(wlst[j].word);
                    char * newword = cptr->add(wlst[j].word, l1);
                    if (newword) {
                       if (nh < maxn) {
                          wlst[nh].word = newword;
                          wlst[nh].allow = cptr->allowCross();
                          wlst[nh].orig = NULL;
                          nh++;
                       } else {
                          free(newword);
                       }
                    }
                }
                cptr = cptr->getFlgNxt();
             }
          }
       }


    
    for (int m = 0; m < al; m ++) {
       const unsigned char c = (unsigned char) (ap[m] & 0x00FF);
       PfxEntry * ptr = pFlag[c];
       while (ptr) {
         if ((ptr->getFlag() == ap[m]) && (!ptr->getKeyLen() || ((badl > ptr->getKeyLen()) &&
                (strncmp(ptr->getKey(), bad, ptr->getKeyLen()) == 0))) &&
                
                !(ptr->getCont() && ((needaffix && 
                      TESTAFF(ptr->getCont(), needaffix, ptr->getContLen())) ||
                     (circumfix && 
                      TESTAFF(ptr->getCont(), circumfix, ptr->getContLen())) ||                      
                  (onlyincompound && 
                      TESTAFF(ptr->getCont(), onlyincompound, ptr->getContLen()))))
                ) {
            char * newword = ptr->add(ts, wl);
            if (newword) {
                if (nh < maxn) {
                    wlst[nh].word = newword;
                    wlst[nh].allow = ptr->allowCross();
                    wlst[nh].orig = NULL;
                    nh++;
                } else {
                    free(newword);
                } 
            }
         }
         ptr = ptr->getFlgNxt();
       }
    }

    return nh;
}


int AffixMgr::get_numrep() const
{
  return numrep;
}


struct replentry * AffixMgr::get_reptable() const
{
  if (! reptable ) return NULL;
  return reptable;
}


RepList * AffixMgr::get_iconvtable() const
{
  if (! iconvtable ) return NULL;
  return iconvtable;
}


RepList * AffixMgr::get_oconvtable() const
{
  if (! oconvtable ) return NULL;
  return oconvtable;
}


struct phonetable * AffixMgr::get_phonetable() const
{
  if (! phone ) return NULL;
  return phone;
}


int AffixMgr::get_nummap() const
{
  return nummap;
}


struct mapentry * AffixMgr::get_maptable() const
{
  if (! maptable ) return NULL;
  return maptable;
}


int AffixMgr::get_numbreak() const
{
  return numbreak;
}


char ** AffixMgr::get_breaktable() const
{
  if (! breaktable ) return NULL;
  return breaktable;
}


char * AffixMgr::get_encoding()
{
  if (! encoding ) encoding = mystrdup(SPELL_ENCODING);
  return mystrdup(encoding);
}


int AffixMgr::get_langnum() const
{
  return langnum;
}


int AffixMgr::get_complexprefixes() const
{
  return complexprefixes;
}


int AffixMgr::get_fullstrip() const
{
  return fullstrip;
}

FLAG AffixMgr::get_keepcase() const
{
  return keepcase;
}

FLAG AffixMgr::get_forceucase() const
{
  return forceucase;
}

FLAG AffixMgr::get_warn() const
{
  return warn;
}

int AffixMgr::get_forbidwarn() const
{
  return forbidwarn;
}

int AffixMgr::get_checksharps() const
{
  return checksharps;
}

char * AffixMgr::encode_flag(unsigned short aflag) const
{
  return pHMgr->encode_flag(aflag);
}



char * AffixMgr::get_ignore() const
{
  if (!ignorechars) return NULL;
  return ignorechars;
}


unsigned short * AffixMgr::get_ignore_utf16(int * len) const
{
  *len = ignorechars_utf16_len;
  return ignorechars_utf16;
}


char * AffixMgr::get_key_string()
{
  if (! keystring ) keystring = mystrdup(SPELL_KEYSTRING);
  return mystrdup(keystring);
}


char * AffixMgr::get_try_string() const
{
  if (! trystring ) return NULL;
  return mystrdup(trystring);
}


const char * AffixMgr::get_wordchars() const
{
  return wordchars;
}

unsigned short * AffixMgr::get_wordchars_utf16(int * len) const
{
  *len = wordchars_utf16_len;
  return wordchars_utf16;
}


int AffixMgr::get_compound() const
{
  return compoundflag || compoundbegin || numdefcpd;
}


FLAG AffixMgr::get_compoundflag() const
{
  return compoundflag;
}


FLAG AffixMgr::get_forbiddenword() const
{
  return forbiddenword;
}


FLAG AffixMgr::get_nosuggest() const
{
  return nosuggest;
}


FLAG AffixMgr::get_nongramsuggest() const
{
  return nongramsuggest;
}


FLAG AffixMgr::get_needaffix() const
{
  return needaffix;
}


FLAG AffixMgr::get_onlyincompound() const
{
  return onlyincompound;
}


FLAG AffixMgr::get_compoundroot() const
{
  return compoundroot;
}


FLAG AffixMgr::get_compoundbegin() const
{
  return compoundbegin;
}


int AffixMgr::get_checknum() const
{
  return checknum;
}


const char * AffixMgr::get_prefix() const
{
  if (pfx) return pfx->getKey();
  return NULL;
}


const char * AffixMgr::get_suffix() const
{
  return sfxappnd;
}


const char * AffixMgr::get_version() const
{
  return version;
}


FLAG AffixMgr::get_lemma_present() const
{
  return lemma_present;
}


struct hentry * AffixMgr::lookup(const char * word)
{
  int i;
  struct hentry * he = NULL;
  for (i = 0; i < *maxdic && !he; i++) {
    he = (alldic[i])->lookup(word);
  }
  return he;
}


int AffixMgr::have_contclass() const
{
  return havecontclass;
}


int AffixMgr::get_utf8() const
{
  return utf8;
}

int AffixMgr::get_maxngramsugs(void) const
{
  return maxngramsugs;
}

int AffixMgr::get_maxcpdsugs(void) const
{
  return maxcpdsugs;
}

int AffixMgr::get_maxdiff(void) const
{
  return maxdiff;
}

int AffixMgr::get_onlymaxdiff(void) const
{
  return onlymaxdiff;
}


int AffixMgr::get_nosplitsugs(void) const
{
  return nosplitsugs;
}


int AffixMgr::get_sugswithdots(void) const
{
  return sugswithdots;
}


int AffixMgr::parse_flag(char * line, unsigned short * out, FileMgr * af) {
   char * s = NULL;
   if (*out != FLAG_NULL && !(*out >= DEFAULTFLAGS)) {
      HUNSPELL_WARNING(stderr, "error: line %d: multiple definitions of an affix file parameter\n", af->getlinenum());
      return 1;
   }
   if (parse_string(line, &s, af->getlinenum())) return 1;
   *out = pHMgr->decode_flag(s);
   free(s);
   return 0;
}


int AffixMgr::parse_num(char * line, int * out, FileMgr * af) {
   char * s = NULL;
   if (*out != -1) {
      HUNSPELL_WARNING(stderr, "error: line %d: multiple definitions of an affix file parameter\n", af->getlinenum());
      return 1;
   }
   if (parse_string(line, &s, af->getlinenum())) return 1;
   *out = atoi(s);
   free(s);
   return 0;
}


int  AffixMgr::parse_cpdsyllable(char * line, FileMgr * af)
{
   char * tp = line;
   char * piece;
   int i = 0;
   int np = 0;
   w_char w[MAXWORDLEN];
   piece = mystrsep(&tp, 0);
   while (piece) {
      if (*piece != '\0') {
          switch(i) {
             case 0: { np++; break; }
             case 1: { cpdmaxsyllable = atoi(piece); np++; break; }
             case 2: {
                if (!utf8) {
                    cpdvowels = mystrdup(piece);
                } else {
                    int n = u8_u16(w, MAXWORDLEN, piece);
                    if (n > 0) {
                        flag_qsort((unsigned short *) w, 0, n);
                        cpdvowels_utf16 = (w_char *) malloc(n * sizeof(w_char));
                        if (!cpdvowels_utf16) return 1;
                        memcpy(cpdvowels_utf16, w, n * sizeof(w_char));
                    }
                    cpdvowels_utf16_len = n;
                }
                np++;
                break;
             }
             default: break;
          }
          i++;
      }
      piece = mystrsep(&tp, 0);
   }
   if (np < 2) {
      HUNSPELL_WARNING(stderr, "error: line %d: missing compoundsyllable information\n", af->getlinenum());
      return 1;
   }
   if (np == 2) cpdvowels = mystrdup("aeiouAEIOU");
   return 0;
}


int  AffixMgr::parse_reptable(char * line, FileMgr * af)
{
   if (numrep != 0) {
      HUNSPELL_WARNING(stderr, "error: line %d: multiple table definitions\n", af->getlinenum());
      return 1;
   }
   char * tp = line;
   char * piece;
   int i = 0;
   int np = 0;
   piece = mystrsep(&tp, 0);
   while (piece) {
       if (*piece != '\0') {
          switch(i) {
             case 0: { np++; break; }
             case 1: { 
                       numrep = atoi(piece);
                       if (numrep < 1) {
                          HUNSPELL_WARNING(stderr, "error: line %d: incorrect entry number\n", af->getlinenum());
                          return 1;
                       }
                       reptable = (replentry *) malloc(numrep * sizeof(struct replentry));
                       if (!reptable) return 1;
                       np++;
                       break;
                     }
             default: break;
          }
          i++;
       }
       piece = mystrsep(&tp, 0);
   }
   if (np != 2) {
      HUNSPELL_WARNING(stderr, "error: line %d: missing data\n", af->getlinenum());
      return 1;
   } 
 
   
   char * nl;
   for (int j=0; j < numrep; j++) {
        if ((nl = af->getline()) == NULL) return 1;
        mychomp(nl);
        tp = nl;
        i = 0;
        reptable[j].pattern = NULL;
        reptable[j].pattern2 = NULL;
        piece = mystrsep(&tp, 0);
        while (piece) {
           if (*piece != '\0') {
               switch(i) {
                  case 0: {
                             if (strncmp(piece,"REP",3) != 0) {
                                 HUNSPELL_WARNING(stderr, "error: line %d: table is corrupt\n", af->getlinenum());
                                 numrep = 0;
                                 return 1;
                             }
                             break;
                          }
                  case 1: {
                            if (*piece == '^') reptable[j].start = true; else reptable[j].start = false;
                            reptable[j].pattern = mystrrep(mystrdup(piece + int(reptable[j].start)),"_"," ");
                            int lr = strlen(reptable[j].pattern) - 1;
                            if (reptable[j].pattern[lr] == '$') {
                                reptable[j].end = true;
                                reptable[j].pattern[lr] = '\0';
                            } else reptable[j].end = false;
                            break;
                          }
                  case 2: { reptable[j].pattern2 = mystrrep(mystrdup(piece),"_"," "); break; }
                  default: break;
               }
               i++;
           }
           piece = mystrsep(&tp, 0);
        }
        if ((!(reptable[j].pattern)) || (!(reptable[j].pattern2))) {
             HUNSPELL_WARNING(stderr, "error: line %d: table is corrupt\n", af->getlinenum());
             numrep = 0;
             return 1;
        }
   }
   return 0;
}


int  AffixMgr::parse_convtable(char * line, FileMgr * af, RepList ** rl, const char * keyword)
{
   if (*rl) {
      HUNSPELL_WARNING(stderr, "error: line %d: multiple table definitions\n", af->getlinenum());
      return 1;
   }
   char * tp = line;
   char * piece;
   int i = 0;
   int np = 0;
   int numrl = 0;
   piece = mystrsep(&tp, 0);
   while (piece) {
       if (*piece != '\0') {
          switch(i) {
             case 0: { np++; break; }
             case 1: { 
                       numrl = atoi(piece);
                       if (numrl < 1) {
                          HUNSPELL_WARNING(stderr, "error: line %d: incorrect entry number\n", af->getlinenum());
                          return 1;
                       }
                       *rl = new RepList(numrl);
                       if (!*rl) return 1;
                       np++;
                       break;
                     }
             default: break;
          }
          i++;
       }
       piece = mystrsep(&tp, 0);
   }
   if (np != 2) {
      HUNSPELL_WARNING(stderr, "error: line %d: missing data\n", af->getlinenum());
      return 1;
   } 
 
   
   char * nl;
   for (int j=0; j < numrl; j++) {
        if (!(nl = af->getline())) return 1;
        mychomp(nl);
        tp = nl;
        i = 0;
        char * pattern = NULL;
        char * pattern2 = NULL;
        piece = mystrsep(&tp, 0);
        while (piece) {
           if (*piece != '\0') {
               switch(i) {
                  case 0: {
                             if (strncmp(piece, keyword, strlen(keyword)) != 0) {
                                 HUNSPELL_WARNING(stderr, "error: line %d: table is corrupt\n", af->getlinenum());
                                 delete *rl;
                                 *rl = NULL;
                                 return 1;
                             }
                             break;
                          }
                  case 1: { pattern = mystrrep(mystrdup(piece),"_"," "); break; }
                  case 2: { 
                    pattern2 = mystrrep(mystrdup(piece),"_"," ");
                    break; 
                  }
                  default: break;
               }
               i++;
           }
           piece = mystrsep(&tp, 0);
        }
        if (!pattern || !pattern2) {
            if (pattern)
                free(pattern);
            if (pattern2)
                free(pattern2);
            HUNSPELL_WARNING(stderr, "error: line %d: table is corrupt\n", af->getlinenum());
            return 1;
        }
        (*rl)->add(pattern, pattern2);
   }
   return 0;
}



int  AffixMgr::parse_phonetable(char * line, FileMgr * af)
{
   if (phone) {
      HUNSPELL_WARNING(stderr, "error: line %d: multiple table definitions\n", af->getlinenum());
      return 1;
   }
   char * tp = line;
   char * piece;
   int i = 0;
   int np = 0;
   piece = mystrsep(&tp, 0);
   while (piece) {
       if (*piece != '\0') {
          switch(i) {
             case 0: { np++; break; }
             case 1: { 
                       phone = (phonetable *) malloc(sizeof(struct phonetable));
                       if (!phone) return 1;
                       phone->num = atoi(piece);
                       phone->rules = NULL;
                       phone->utf8 = (char) utf8;
                       if (phone->num < 1) {
                          HUNSPELL_WARNING(stderr, "error: line %d: bad entry number\n", af->getlinenum());
                          return 1;
                       }
                       phone->rules = (char * *) malloc(2 * (phone->num + 1) * sizeof(char *));
                       if (!phone->rules) {
                          free(phone);
                          phone = NULL;
                          return 1;
                       }
                       np++;
                       break;
                     }
             default: break;
          }
          i++;
       }
       piece = mystrsep(&tp, 0);
   }
   if (np != 2) {
      HUNSPELL_WARNING(stderr, "error: line %d: missing data\n", af->getlinenum());
      return 1;
   } 
 
   
   char * nl;
   for (int j=0; j < phone->num; j++) {
        if (!(nl = af->getline())) return 1;
        mychomp(nl);
        tp = nl;
        i = 0;
        phone->rules[j * 2] = NULL;
        phone->rules[j * 2 + 1] = NULL;
        piece = mystrsep(&tp, 0);
        while (piece) {
           if (*piece != '\0') {
               switch(i) {
                  case 0: {
                             if (strncmp(piece,"PHONE",5) != 0) {
                                 HUNSPELL_WARNING(stderr, "error: line %d: table is corrupt\n", af->getlinenum());
                                 phone->num = 0;
                                 return 1;
                             }
                             break;
                          }
                  case 1: { phone->rules[j * 2] = mystrrep(mystrdup(piece),"_",""); break; }
                  case 2: { phone->rules[j * 2 + 1] = mystrrep(mystrdup(piece),"_",""); break; }
                  default: break;
               }
               i++;
           }
           piece = mystrsep(&tp, 0);
        }
        if ((!(phone->rules[j * 2])) || (!(phone->rules[j * 2 + 1]))) {
             HUNSPELL_WARNING(stderr, "error: line %d: table is corrupt\n", af->getlinenum());
             phone->num = 0;
             return 1;
        }
   }
   phone->rules[phone->num * 2] = mystrdup("");
   phone->rules[phone->num * 2 + 1] = mystrdup("");
   init_phonet_hash(*phone);
   return 0;
}


int  AffixMgr::parse_checkcpdtable(char * line, FileMgr * af)
{
   if (numcheckcpd != 0) {
      HUNSPELL_WARNING(stderr, "error: line %d: multiple table definitions\n", af->getlinenum());
      return 1;
   }
   char * tp = line;
   char * piece;
   int i = 0;
   int np = 0;
   piece = mystrsep(&tp, 0);
   while (piece) {
       if (*piece != '\0') {
          switch(i) {
             case 0: { np++; break; }
             case 1: { 
                       numcheckcpd = atoi(piece);
                       if (numcheckcpd < 1) {
                          HUNSPELL_WARNING(stderr, "error: line %d: bad entry number\n", af->getlinenum());
                          return 1;
                       }
                       checkcpdtable = (patentry *) malloc(numcheckcpd * sizeof(struct patentry));
                       if (!checkcpdtable) return 1;
                       np++;
                       break;
                     }
             default: break;
          }
          i++;
       }
       piece = mystrsep(&tp, 0);
   }
   if (np != 2) {
      HUNSPELL_WARNING(stderr, "error: line %d: missing data\n",  af->getlinenum());
      return 1;
   }

   
   char * nl;
   for (int j=0; j < numcheckcpd; j++) {
        if (!(nl = af->getline())) return 1;
        mychomp(nl);
        tp = nl;
        i = 0;
        checkcpdtable[j].pattern = NULL;
        checkcpdtable[j].pattern2 = NULL;
        checkcpdtable[j].pattern3 = NULL;
        checkcpdtable[j].cond = FLAG_NULL;
        checkcpdtable[j].cond2 = FLAG_NULL;
        piece = mystrsep(&tp, 0);
        while (piece) {
           if (*piece != '\0') {
               switch(i) {
                  case 0: {
                             if (strncmp(piece,"CHECKCOMPOUNDPATTERN",20) != 0) {
                                 HUNSPELL_WARNING(stderr, "error: line %d: table is corrupt\n", af->getlinenum());
                                 numcheckcpd = 0;
                                 return 1;
                             }
                             break;
                          }
                  case 1: { 
                    checkcpdtable[j].pattern = mystrdup(piece); 
                    char * p = strchr(checkcpdtable[j].pattern, '/');
                    if (p) {
                      *p = '\0';
                    checkcpdtable[j].cond = pHMgr->decode_flag(p + 1);
                    }
                    break; }
                  case 2: { 
                    checkcpdtable[j].pattern2 = mystrdup(piece);
                    char * p = strchr(checkcpdtable[j].pattern2, '/');
                    if (p) {
                      *p = '\0';
                      checkcpdtable[j].cond2 = pHMgr->decode_flag(p + 1);
                    }
                    break;
                    }
                  case 3: { checkcpdtable[j].pattern3 = mystrdup(piece); simplifiedcpd = 1; break; }
                  default: break;
               }
               i++;
           }
           piece = mystrsep(&tp, 0);
        }
        if ((!(checkcpdtable[j].pattern)) || (!(checkcpdtable[j].pattern2))) {
             HUNSPELL_WARNING(stderr, "error: line %d: table is corrupt\n", af->getlinenum());
             numcheckcpd = 0;
             return 1;
        }
   }
   return 0;
}


int  AffixMgr::parse_defcpdtable(char * line, FileMgr * af)
{
   if (numdefcpd != 0) {
      HUNSPELL_WARNING(stderr, "error: line %d: multiple table definitions\n", af->getlinenum());
      return 1;
   }
   char * tp = line;
   char * piece;
   int i = 0;
   int np = 0;
   piece = mystrsep(&tp, 0);
   while (piece) {
       if (*piece != '\0') {
          switch(i) {
             case 0: { np++; break; }
             case 1: { 
                       numdefcpd = atoi(piece);
                       if (numdefcpd < 1) {
                          HUNSPELL_WARNING(stderr, "error: line %d: bad entry number\n", af->getlinenum());
                          return 1;
                       }
                       defcpdtable = (flagentry *) malloc(numdefcpd * sizeof(flagentry));
                       if (!defcpdtable) return 1;
                       np++;
                       break;
                     }
             default: break;
          }
          i++;
       }
       piece = mystrsep(&tp, 0);
   }
   if (np != 2) {
      HUNSPELL_WARNING(stderr, "error: line %d: missing data\n", af->getlinenum());
      return 1;
   } 
 
   
   char * nl;
   for (int j=0; j < numdefcpd; j++) {
        if (!(nl = af->getline())) return 1;
        mychomp(nl);
        tp = nl;
        i = 0;
        defcpdtable[j].def = NULL;
        piece = mystrsep(&tp, 0);
        while (piece) {
           if (*piece != '\0') {
               switch(i) {
                  case 0: {
                             if (strncmp(piece, "COMPOUNDRULE", 12) != 0) {
                                 HUNSPELL_WARNING(stderr, "error: line %d: table is corrupt\n", af->getlinenum());
                                 numdefcpd = 0;
                                 return 1;
                             }
                             break;
                          }
                  case 1: { 
                            if (strchr(piece, '(')) {
                                defcpdtable[j].def = (FLAG *) malloc(strlen(piece) * sizeof(FLAG));
                                defcpdtable[j].len = 0;
                                int end = 0;
                                FLAG * conv;
                                while (!end) {
                                    char * par = piece + 1;
                                    while (*par != '(' && *par != ')' && *par != '\0') par++;
                                    if (*par == '\0') end = 1; else *par = '\0';
                                    if (*piece == '(') piece++;
                                    if (*piece == '*' || *piece == '?') {
                                        defcpdtable[j].def[defcpdtable[j].len++] = (FLAG) *piece;
                                    } else if (*piece != '\0') {
                                        int l = pHMgr->decode_flags(&conv, piece, af);
                                        for (int k = 0; k < l; k++) defcpdtable[j].def[defcpdtable[j].len++] = conv[k];
                                        free(conv);
                                    }
                                    piece = par + 1;
                                }
                            } else {
                                defcpdtable[j].len = pHMgr->decode_flags(&(defcpdtable[j].def), piece, af);
                            }
                            break; 
                           }
                  default: break;
               }
               i++;
           }
           piece = mystrsep(&tp, 0);
        }
        if (!defcpdtable[j].len) {
             HUNSPELL_WARNING(stderr, "error: line %d: table is corrupt\n", af->getlinenum());
             numdefcpd = 0;
             return 1;
        }
   }
   return 0;
}



int  AffixMgr::parse_maptable(char * line, FileMgr * af)
{
   if (nummap != 0) {
      HUNSPELL_WARNING(stderr, "error: line %d: multiple table definitions\n", af->getlinenum());
      return 1;
   }
   char * tp = line;
   char * piece;
   int i = 0;
   int np = 0;
   piece = mystrsep(&tp, 0);
   while (piece) {
       if (*piece != '\0') {
          switch(i) {
             case 0: { np++; break; }
             case 1: { 
                       nummap = atoi(piece);
                       if (nummap < 1) {
                          HUNSPELL_WARNING(stderr, "error: line %d: bad entry number\n", af->getlinenum());
                          return 1;
                       }
                       maptable = (mapentry *) malloc(nummap * sizeof(struct mapentry));
                       if (!maptable) return 1;
                       np++;
                       break;
                     }
             default: break;
          }
          i++;
       }
       piece = mystrsep(&tp, 0);
   }
   if (np != 2) {
      HUNSPELL_WARNING(stderr, "error: line %d: missing data\n", af->getlinenum());
      return 1;
   } 
 
   
   char * nl;
   for (int j=0; j < nummap; j++) {
        if (!(nl = af->getline())) return 1;
        mychomp(nl);
        tp = nl;
        i = 0;
        maptable[j].set = NULL;
        maptable[j].len = 0;
        piece = mystrsep(&tp, 0);
        while (piece) {
           if (*piece != '\0') {
               switch(i) {
                  case 0: {
                             if (strncmp(piece,"MAP",3) != 0) {
                                 HUNSPELL_WARNING(stderr, "error: line %d: table is corrupt\n", af->getlinenum());
                                 nummap = 0;
                                 return 1;
                             }
                             break;
                          }
                  case 1: {
			    int setn = 0;
                            maptable[j].len = strlen(piece);
                            maptable[j].set = (char **) malloc(maptable[j].len * sizeof(char*));
                            if (!maptable[j].set) return 1;
			    for (int k = 0; k < maptable[j].len; k++) {
				int chl = 1;
				int chb = k;
			        if (piece[k] == '(') {
				    char * parpos = strchr(piece + k, ')');
				    if (parpos != NULL) {
					chb = k + 1;
					chl = (int)(parpos - piece) - k - 1;
					k = k + chl + 1;
				    }
				} else {
				    if (utf8 && (piece[k] & 0xc0) == 0xc0) {
					for (k++; utf8 && (piece[k] & 0xc0) == 0x80; k++);
					chl = k - chb;
					k--;
				    }
				}
				maptable[j].set[setn] = (char *) malloc(chl + 1);
				if (!maptable[j].set[setn]) return 1;
				strncpy(maptable[j].set[setn], piece + chb, chl);
				maptable[j].set[setn][chl] = '\0';
				setn++;
			    }
                            maptable[j].len = setn;
                            break; }
                  default: break;
               }
               i++;
           }
           piece = mystrsep(&tp, 0);
        }
        if (!maptable[j].set || !maptable[j].len) {
             HUNSPELL_WARNING(stderr, "error: line %d: table is corrupt\n", af->getlinenum());
             nummap = 0;
             return 1;
        }
   }
   return 0;
}


int  AffixMgr::parse_breaktable(char * line, FileMgr * af)
{
   if (numbreak > -1) {
      HUNSPELL_WARNING(stderr, "error: line %d: multiple table definitions\n", af->getlinenum());
      return 1;
   }
   char * tp = line;
   char * piece;
   int i = 0;
   int np = 0;
   piece = mystrsep(&tp, 0);
   while (piece) {
       if (*piece != '\0') {
          switch(i) {
             case 0: { np++; break; }
             case 1: { 
                       numbreak = atoi(piece);
                       if (numbreak < 0) {
                          HUNSPELL_WARNING(stderr, "error: line %d: bad entry number\n", af->getlinenum());
                          return 1;
                       }
                       if (numbreak == 0) return 0;
                       breaktable = (char **) malloc(numbreak * sizeof(char *));
                       if (!breaktable) return 1;
                       np++;
                       break;
                     }
             default: break;
          }
          i++;
       }
       piece = mystrsep(&tp, 0);
   }
   if (np != 2) {
      HUNSPELL_WARNING(stderr, "error: line %d: missing data\n", af->getlinenum());
      return 1;
   } 
 
   
   char * nl;
   for (int j=0; j < numbreak; j++) {
        if (!(nl = af->getline())) return 1;
        mychomp(nl);
        tp = nl;
        i = 0;
        piece = mystrsep(&tp, 0);
        while (piece) {
           if (*piece != '\0') {
               switch(i) {
                  case 0: {
                             if (strncmp(piece,"BREAK",5) != 0) {
                                 HUNSPELL_WARNING(stderr, "error: line %d: table is corrupt\n", af->getlinenum());
                                 numbreak = 0;
                                 return 1;
                             }
                             break;
                          }
                  case 1: {
                            breaktable[j] = mystrdup(piece);
                            break;
                          }
                  default: break;
               }
               i++;
           }
           piece = mystrsep(&tp, 0);
        }
        if (!breaktable) {
             HUNSPELL_WARNING(stderr, "error: line %d: table is corrupt\n", af->getlinenum());
             numbreak = 0;
             return 1;
        }
   }
   return 0;
}

void AffixMgr::reverse_condition(char * piece) {
    int neg = 0;
    for (char * k = piece + strlen(piece) - 1; k >= piece; k--) {
        switch(*k) {
          case '[': {
                if (neg) *(k+1) = '['; else *k = ']';
                    break;
            }
          case ']': {
                *k = '[';
                if (neg) *(k+1) = '^';
                neg = 0;
                break;
            }
          case '^': {
               if (*(k+1) == ']') neg = 1; else *(k+1) = *k;
               break;
                }
          default: {
            if (neg) *(k+1) = *k;
          }
       }
    }
}

int  AffixMgr::parse_affix(char * line, const char at, FileMgr * af, char * dupflags)
{
   int numents = 0;      

   unsigned short aflag = 0;      

   char ff=0;
   std::vector<affentry> affentries;

   char * tp = line;
   char * nl = line;
   char * piece;
   int i = 0;

   
#ifdef DEBUG
   int basefieldnum = 0;
#endif

   

   int np = 0;

   piece = mystrsep(&tp, 0);
   while (piece) {
      if (*piece != '\0') {
          switch(i) {
             
             case 0: { np++; break; }
          
             
             case 1: { 
                    np++;
                    aflag = pHMgr->decode_flag(piece);
                    if (((at == 'S') && (dupflags[aflag] & dupSFX)) ||
                        ((at == 'P') && (dupflags[aflag] & dupPFX))) {
                        HUNSPELL_WARNING(stderr, "error: line %d: multiple definitions of an affix flag\n",
                            af->getlinenum());
                        
                    }
                    dupflags[aflag] += (char) ((at == 'S') ? dupSFX : dupPFX);
                    break; 
                    }
             
             case 2: { np++; if (*piece == 'Y') ff = aeXPRODUCT; break; }

             
             case 3: { 
                       np++;
                       numents = atoi(piece); 
                       if (numents == 0) {
                           char * err = pHMgr->encode_flag(aflag);
                           if (err) {
                                HUNSPELL_WARNING(stderr, "error: line %d: bad entry number\n",
                                   af->getlinenum());
                                free(err);
                           }
                           return 1;
                       }
                       affentries.resize(numents);
                       affentries[0].opts = ff;
                       if (utf8) affentries[0].opts += aeUTF8;
                       if (pHMgr->is_aliasf()) affentries[0].opts += aeALIASF;
                       if (pHMgr->is_aliasm()) affentries[0].opts += aeALIASM;
                       affentries[0].aflag = aflag;
                     }

             default: break;
          }
          i++;
      }
      piece = mystrsep(&tp, 0);
   }
   
   if (np != 4) {
       char * err = pHMgr->encode_flag(aflag);
       if (err) {
            HUNSPELL_WARNING(stderr, "error: line %d: missing data\n", af->getlinenum());
            free(err);
       }
       return 1;
   }
 
   
   std::vector<affentry>::iterator start = affentries.begin();
   std::vector<affentry>::iterator end = affentries.end();
   for (std::vector<affentry>::iterator entry = start; entry != end; ++entry) {
      if ((nl = af->getline()) == NULL) return 1;
      mychomp(nl);
      tp = nl;
      i = 0;
      np = 0;

      
      piece = mystrsep(&tp, 0);
      while (piece) {
         if (*piece != '\0') {
             switch(i) {
                
                case 0: { 
                          np++;
                          if (entry != start) entry->opts = start->opts &
                             (char) (aeXPRODUCT + aeUTF8 + aeALIASF + aeALIASM);
                          break;
                        }

                
                case 1: { 
                          np++;
                          if (pHMgr->decode_flag(piece) != aflag) {
                              char * err = pHMgr->encode_flag(aflag);
                              if (err) {
                                HUNSPELL_WARNING(stderr, "error: line %d: affix %s is corrupt\n",
                                    af->getlinenum(), err);
                                free(err);
                              }
                              return 1;
                          }

                          if (entry != start) entry->aflag = start->aflag;
                          break;
                        }

                
                case 2: { 
                          np++;
                          if (complexprefixes) {
                            if (utf8) reverseword_utf(piece); else reverseword(piece);
                          }
                          entry->strip = mystrdup(piece);
                          entry->stripl = (unsigned char) strlen(entry->strip);
                          if (strcmp(entry->strip,"0") == 0) {
                              free(entry->strip);
                              entry->strip=mystrdup("");
                              entry->stripl = 0;
                          }   
                          break; 
                        }

                
                case 3: { 
                          char * dash;  
                          entry->morphcode = NULL;
                          entry->contclass = NULL;
                          entry->contclasslen = 0;
                          np++;
                          dash = strchr(piece, '/');
                          if (dash) {
                            *dash = '\0';

                            if (ignorechars) {
                              if (utf8) {
                                remove_ignored_chars_utf(piece, ignorechars_utf16, ignorechars_utf16_len);
                              } else {
                                remove_ignored_chars(piece,ignorechars);
                              }
                            }

                            if (complexprefixes) {
                                if (utf8) reverseword_utf(piece); else reverseword(piece);
                            }
                            entry->appnd = mystrdup(piece);

                            if (pHMgr->is_aliasf()) {
                                int index = atoi(dash + 1);
                                entry->contclasslen = (unsigned short) pHMgr->get_aliasf(index, &(entry->contclass), af);
                                if (!entry->contclasslen) HUNSPELL_WARNING(stderr, "error: bad affix flag alias: \"%s\"\n", dash+1);
                            } else {
                                entry->contclasslen = (unsigned short) pHMgr->decode_flags(&(entry->contclass), dash + 1, af);
                                flag_qsort(entry->contclass, 0, entry->contclasslen);
                            }
                            *dash = '/';

                            havecontclass = 1;
                            for (unsigned short _i = 0; _i < entry->contclasslen; _i++) {
                              contclasses[(entry->contclass)[_i]] = 1;
                            }
                          } else {
                            if (ignorechars) {
                              if (utf8) {
                                remove_ignored_chars_utf(piece, ignorechars_utf16, ignorechars_utf16_len);
                              } else {
                                remove_ignored_chars(piece,ignorechars);
                              }
                            }

                            if (complexprefixes) {
                                if (utf8) reverseword_utf(piece); else reverseword(piece);
                            }
                            entry->appnd = mystrdup(piece);
                          }

                          entry->appndl = (unsigned char) strlen(entry->appnd);
                          if (strcmp(entry->appnd,"0") == 0) {
                              free(entry->appnd);
                              entry->appnd=mystrdup("");
                              entry->appndl = 0;
                          }   
                          break; 
                        }

                
                case 4: { 
                          np++;
                          if (complexprefixes) {
                            if (utf8) reverseword_utf(piece); else reverseword(piece);
                            reverse_condition(piece);
                          }
                          if (entry->stripl && (strcmp(piece, ".") != 0) &&
                            redundant_condition(at, entry->strip, entry->stripl, piece, af->getlinenum()))
                                strcpy(piece, ".");
                          if (at == 'S') {
                            reverseword(piece);
                            reverse_condition(piece);
                          }
                          if (encodeit(*entry, piece)) return 1;
                         break;
                }

                case 5: {
                          np++;
                          if (pHMgr->is_aliasm()) {
                            int index = atoi(piece);
                            entry->morphcode = pHMgr->get_aliasm(index);
                          } else {
                            if (complexprefixes) { 
                                if (utf8) reverseword_utf(piece); else reverseword(piece);
                            }
                            
                            if (*tp) {
                                *(tp - 1) = ' ';
                                tp = tp + strlen(tp);
                            }
                            entry->morphcode = mystrdup(piece);
                            if (!entry->morphcode) return 1;
                          }
                          break; 
                }
                default: break;
             }
             i++;
         }
         piece = mystrsep(&tp, 0);
      }
      
      if (np < 4) {
          char * err = pHMgr->encode_flag(aflag);
          if (err) {
            HUNSPELL_WARNING(stderr, "error: line %d: affix %s is corrupt\n",
                af->getlinenum(), err);
            free(err);
          }
          return 1;
      }

#ifdef DEBUG
      
      if (basefieldnum) {
        int fieldnum = !(entry->morphcode) ? 5 : ((*(entry->morphcode)=='#') ? 5 : 6);
          if (fieldnum != basefieldnum) 
            HUNSPELL_WARNING(stderr, "warning: line %d: bad field number\n", af->getlinenum());
      } else {
        basefieldnum = !(entry->morphcode) ? 5 : ((*(entry->morphcode)=='#') ? 5 : 6);
      }
#endif
   }
 
   
   
   for (std::vector<affentry>::iterator entry = start; entry != end; ++entry) {
      if (at == 'P') {
          PfxEntry * pfxptr = new PfxEntry(this,&(*entry));
          build_pfxtree(pfxptr);
      } else {
          SfxEntry * sfxptr = new SfxEntry(this,&(*entry));
          build_sfxtree(sfxptr); 
      }
   }
   return 0;
}

int AffixMgr::redundant_condition(char ft, char * strip, int stripl, const char * cond, int linenum) {
  int condl = strlen(cond);
  int i;
  int j;
  int neg;
  int in;
  if (ft == 'P') { 
    if (strncmp(strip, cond, condl) == 0) return 1;
    if (utf8) {
    } else {
      for (i = 0, j = 0; (i < stripl) && (j < condl); i++, j++) {
        if (cond[j] != '[') {
          if (cond[j] != strip[i]) {
            HUNSPELL_WARNING(stderr, "warning: line %d: incompatible stripping characters and condition\n", linenum);
            return 0;
          }
        } else {
          neg = (cond[j+1] == '^') ? 1 : 0;
          in = 0;
          do {
            j++;
            if (strip[i] == cond[j]) in = 1;
          } while ((j < (condl - 1)) && (cond[j] != ']'));
          if (j == (condl - 1) && (cond[j] != ']')) {
            HUNSPELL_WARNING(stderr, "error: line %d: missing ] in condition:\n%s\n", linenum, cond);
            return 0;
          }
          if ((!neg && !in) || (neg && in)) {
            HUNSPELL_WARNING(stderr, "warning: line %d: incompatible stripping characters and condition\n", linenum);
            return 0;
          }
        }
      }
      if (j >= condl) return 1;
    }
  } else { 
    if ((stripl >= condl) && strcmp(strip + stripl - condl, cond) == 0) return 1;
    if (utf8) {
    } else {
      for (i = stripl - 1, j = condl - 1; (i >= 0) && (j >= 0); i--, j--) {
        if (cond[j] != ']') {
          if (cond[j] != strip[i]) {
            HUNSPELL_WARNING(stderr, "warning: line %d: incompatible stripping characters and condition\n", linenum);
            return 0;
          }
        } else {
          in = 0;
          do {
            j--;
            if (strip[i] == cond[j]) in = 1;
          } while ((j > 0) && (cond[j] != '['));
          if ((j == 0) && (cond[j] != '[')) {
            HUNSPELL_WARNING(stderr, "error: line: %d: missing ] in condition:\n%s\n", linenum, cond);
            return 0;
          }
          neg = (cond[j+1] == '^') ? 1 : 0;
          if ((!neg && !in) || (neg && in)) {
            HUNSPELL_WARNING(stderr, "warning: line %d: incompatible stripping characters and condition\n", linenum);
            return 0;
          }
        }
      }
      if (j < 0) return 1;
    }
  }
  return 0;
}
