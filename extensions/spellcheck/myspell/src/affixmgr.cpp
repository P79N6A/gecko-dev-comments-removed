#include "license.readme"

#include <stdlib.h> 
#include <string.h>
#include <stdio.h> 

#include "affixmgr.hxx"
#include "affentry.hxx"





extern void   mychomp(char * s);
extern char * mystrdup(const char * s);
extern char * myrevstrdup(const char * s);
extern char * mystrsep(char ** sptr, const char delim);
extern int    isSubset(const char * s1, const char * s2); 
extern int    isRevSubset(const char * s1, const char * end_of_s2, int len_s2); 


AffixMgr::AffixMgr(const char * affpath, HashMgr* ptr) 
{
  
  pHMgr = ptr;
  trystring = NULL;
  encoding=NULL;
  reptable = NULL;
  numrep = 0;
  maptable = NULL;
  nummap = 0;
  compound=NULL;
  nosplitsugs= (0==1);

  cpdmin = 3;  
  for (int i=0; i < SETSIZE; i++) {
    pStart[i] = NULL;
    sStart[i] = NULL;
    pFlag[i] = NULL;
    sFlag[i] = NULL;
  }
  if (parse_file(affpath)) {
    fprintf(stderr,"Failure loading aff file %s\n",affpath);
    fflush(stderr);
  }
}


AffixMgr::~AffixMgr() 
{
 
  
  for (int i=0; i < SETSIZE ;i++) {
    pFlag[i] = NULL;
    PfxEntry * ptr = (PfxEntry *)pStart[i];
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
    SfxEntry * ptr = (SfxEntry *)sStart[j];
    SfxEntry * nptr = NULL;
    while (ptr) {
      nptr = ptr->getNext();
      delete(ptr);
      ptr = nptr;
      nptr = NULL;
    }  
  }

  if (trystring) free(trystring);
  trystring=NULL;
  if (encoding) free(encoding);
  encoding=NULL;
  if (maptable) {  
    for (int j=0; j < nummap; j++) {
      free(maptable[j].set);
      maptable[j].set = NULL;
      maptable[j].len = 0;
    }
    free(maptable);  
    maptable = NULL;
  }
  nummap = 0;
  if (reptable) {  
    for (int j=0; j < numrep; j++) {
      free(reptable[j].pattern);
      free(reptable[j].replacement);
      reptable[j].pattern = NULL;
      reptable[j].replacement = NULL;
    }
    free(reptable);  
    reptable = NULL;
  }
  numrep = 0;
  if (compound) free(compound);
  compound=NULL;
  pHMgr = NULL;
  cpdmin = 0;
}



int  AffixMgr::parse_file(const char * affpath)
{

  
  char line[MAXLNLEN+1];
 
  
  char ft;

  
  FILE * afflst;
  afflst = fopen(affpath,"r");
  if (!afflst) {
    fprintf(stderr,"Error - could not open affix description file %s\n",affpath);
    return 1;
  }

  
  


  
  

  while (fgets(line,MAXLNLEN,afflst)) {
    mychomp(line);

    
    if (strncmp(line,"TRY",3) == 0) {
      if (parse_try(line)) {
        fclose(afflst);
        return 1;
      }
    }

    
    if (strncmp(line,"SET",3) == 0) {
      if (parse_set(line)) {
        fclose(afflst);
        return 1;
      }
    }

    
    if (strncmp(line,"COMPOUNDFLAG",12) == 0) {
      if (parse_cpdflag(line)) {
        fclose(afflst);
        return 1;
      }
    }

    
    if (strncmp(line,"COMPOUNDMIN",11) == 0) {
      if (parse_cpdmin(line)) {
        fclose(afflst);
        return 1;
      }
    }

    
    if (strncmp(line,"REP",3) == 0) {
      if (parse_reptable(line, afflst)) {
        fclose(afflst);
        return 1;
      }
    }

    
    if (strncmp(line,"MAP",3) == 0) {
      if (parse_maptable(line, afflst)) {
        fclose(afflst);
        return 1;
      }
    }

    
    ft = ' ';
    if (strncmp(line,"PFX",3) == 0) ft = 'P';
    if (strncmp(line,"SFX",3) == 0) ft = 'S';
    if (ft != ' ') {
      if (parse_affix(line, ft, afflst)) {
        fclose(afflst);
        return 1;
      }
    }

    
    if (strncmp(line,"NOSPLITSUGS",11) == 0)
      nosplitsugs=(0==0);

  }
  fclose(afflst);

  
  process_pfx_tree_to_list();
  process_sfx_tree_to_list();

  
  

  
  

  
  
  
  
  

  
  
  

  
  
  

  
  

  process_pfx_order();
  process_sfx_order();

  return 0;
}






int AffixMgr::build_pfxtree(AffEntry* pfxptr)
{
  PfxEntry * ptr;
  PfxEntry * pptr;
  PfxEntry * ep = (PfxEntry*) pfxptr;

  
  const char * key = ep->getKey();
  const unsigned char flg = ep->getFlag();

  
  ptr = (PfxEntry*)pFlag[flg];
  ep->setFlgNxt(ptr);
  pFlag[flg] = (AffEntry *) ep;


  
  if (strlen(key) == 0) {
    
    ptr = (PfxEntry*)pStart[0];
    ep->setNext(ptr);
    pStart[0] = (AffEntry*)ep;
    return 0;
  }

  
  ep->setNextEQ(NULL);
  ep->setNextNE(NULL);

  unsigned char sp = *((const unsigned char *)key);
  ptr = (PfxEntry*)pStart[sp];
  
  
  if (!ptr) {
    pStart[sp] = (AffEntry*)ep;
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






int AffixMgr::build_sfxtree(AffEntry* sfxptr)
{
  SfxEntry * ptr;
  SfxEntry * pptr;
  SfxEntry * ep = (SfxEntry *) sfxptr;

  
  const char * key = ep->getKey();
  const unsigned char flg = ep->getFlag();

  
  ptr = (SfxEntry*)sFlag[flg];
  ep->setFlgNxt(ptr);
  sFlag[flg] = (AffEntry *) ep;


  

  
  if (strlen(key) == 0) {
    
    ptr = (SfxEntry*)sStart[0];
    ep->setNext(ptr);
    sStart[0] = (AffEntry*)ep;
    return 0;
  }

  
  ep->setNextEQ(NULL);
  ep->setNextNE(NULL);

  unsigned char sp = *((const unsigned char *)key);
  ptr = (SfxEntry*)sStart[sp];
  
  
  if (!ptr) {
    sStart[sp] = (AffEntry*)ep;
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


AffEntry* AffixMgr::process_pfx_in_order(AffEntry* ptr, AffEntry* nptr)
{
  if (ptr) {
    nptr = process_pfx_in_order(((PfxEntry*) ptr)->getNextNE(), nptr);
    ((PfxEntry*) ptr)->setNext((PfxEntry*) nptr);
    nptr = process_pfx_in_order(((PfxEntry*) ptr)->getNextEQ(), ptr);
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

AffEntry* AffixMgr::process_sfx_in_order(AffEntry* ptr, AffEntry* nptr)
{
  if (ptr) {
    nptr = process_sfx_in_order(((SfxEntry*) ptr)->getNextNE(), nptr);
    ((SfxEntry*) ptr)->setNext((SfxEntry*) nptr);
    nptr = process_sfx_in_order(((SfxEntry*) ptr)->getNextEQ(), ptr);
  }
  return nptr;
}





int AffixMgr::process_pfx_order()
{
  PfxEntry* ptr;

  
  for (int i=1; i < SETSIZE; i++) {

    ptr = (PfxEntry*)pStart[i];

    
    
    
    
    
    

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

    
    
    
    

    ptr = (PfxEntry *) pStart[i];
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

    ptr = (SfxEntry *) sStart[i];

    
    
    
    
    
    

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


    
    
    
    

    ptr = (SfxEntry *) sStart[i];
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








void AffixMgr::encodeit(struct affentry * ptr, char * cs)
{
  unsigned char c;
  int i, j, k;
  unsigned char mbr[MAXLNLEN];

  
  for (i=0;i<SETSIZE;i++) ptr->conds[i] = (unsigned char) 0;

  
  int nc = strlen(cs);
  int neg = 0;   
  int grp = 0;   
  int n = 0;     
  int ec = 0;    
  int nm = 0;    

  
  if (strcmp(cs,".")==0) {
    ptr->numconds = 0;
    return;
  }

  i = 0;
  while (i < nc) {
    c = *((unsigned char *)(cs + i));

    
    if (c == '[') {
      grp = 1;
      c = 0;
    }

    
    if ((grp == 1) && (c == '^')) {
      neg = 1;
      c = 0;
    }

    
    if (c == ']') {
      ec = 1;
       c = 0;
    }

    
    if ((grp == 1) && (c != 0)) {
      *(mbr + nm) = c;
      nm++;
      c = 0;
    }

    
    if (c != 0) {
      ec = 1;
    }

    
    if (ec) {
      if (grp == 1) {
        if (neg == 0) {
          
      for (j=0;j<nm;j++) {
            k = (unsigned int) mbr[j];
            ptr->conds[k] = ptr->conds[k] | (1 << n);
          }
    } else {
      
          for (j=0;j<SETSIZE;j++) ptr->conds[j] = ptr->conds[j] | (1 << n);
          for (j=0;j<nm;j++) {
            k = (unsigned int) mbr[j];
            ptr->conds[k] = ptr->conds[k] & ~(1 << n);
          }
        }
        neg = 0;
        grp = 0;   
        nm = 0;
      } else {
        
        
        if (c == '.') {
          
          for (j=0;j<SETSIZE;j++) ptr->conds[j] = ptr->conds[j] | (1 << n);
        } else {  
          ptr->conds[(unsigned int) c] = ptr->conds[(unsigned int)c] | (1 << n);
        }
      }
      n++;
      ec = 0;
    }


    i++;
  }
  ptr->numconds = n;
  return;
}



struct hentry * AffixMgr::prefix_check (const char * word, int len)
{
  struct hentry * rv= NULL;
 
  
  PfxEntry * pe = (PfxEntry *) pStart[0];
  while (pe) {
    rv = pe->check(word,len);
    if (rv) return rv;
    pe = pe->getNext();
  }
  
  
  unsigned char sp = *((const unsigned char *)word);
  PfxEntry * pptr = (PfxEntry *)pStart[sp];

  while (pptr) {
    if (isSubset(pptr->getKey(),word)) {
      rv = pptr->check(word,len);
      if (rv) return rv;
      pptr = pptr->getNextEQ();
    } else {
      pptr = pptr->getNextNE();
    }
  }
    
  return NULL;
}


struct hentry * AffixMgr::compound_check (const char * word, int len, char compound_flag)
{
  int i;
  struct hentry * rv= NULL;
  char * st;
  char ch;
    
  
  if (len < cpdmin) return NULL;

  st = mystrdup(word);
    
  for (i=cpdmin; i < (len - (cpdmin-1)); i++) {

    ch = st[i];
    st[i] = '\0';

    rv = lookup(st);
    if (!rv) rv = affix_check(st,i);

    if ((rv) && (TESTAFF(rv->astr, compound_flag, rv->alen))) {
      rv = lookup((word+i));
      if ((rv) && (TESTAFF(rv->astr, compound_flag, rv->alen))) {
        free(st);
        return rv;
      }
      rv = affix_check((word+i),strlen(word+i));
      if ((rv) && (TESTAFF(rv->astr, compound_flag, rv->alen))) {
        free(st);
        return rv;
      }
      rv = compound_check((word+i),strlen(word+i),compound_flag); 
      if (rv) {
        free(st);
        return rv;
      }
        
    }
    st[i] = ch;
  }
  free(st);
  return NULL;
}    




struct hentry * AffixMgr::suffix_check (const char * word, int len, 
                       int sfxopts, AffEntry * ppfx)
{
  struct hentry * rv = NULL;

  
  SfxEntry * se = (SfxEntry *) sStart[0];
  while (se) {
    rv = se->check(word,len, sfxopts, ppfx);
    if (rv) return rv;
    se = se->getNext();
  }
  
  
  unsigned char sp = *((const unsigned char *)(word + len - 1));


  SfxEntry * sptr = (SfxEntry *) sStart[sp];

  while (sptr) {
    if (isRevSubset(sptr->getKey(),(word+len-1), len)) {
      rv = sptr->check(word,len, sfxopts, ppfx);
      if (rv) {
        return rv;
      }
      sptr = sptr->getNextEQ();
    } else {
      sptr = sptr->getNextNE();
    }
  }
  return NULL;
}




struct hentry * AffixMgr::affix_check (const char * word, int len)
{
    struct hentry * rv= NULL;

    
    rv = prefix_check(word, len);
    if (rv) return rv;

    
    rv = suffix_check(word, len, 0, NULL);
    return rv;
}


int AffixMgr::expand_rootword(struct guessword * wlst, int maxn, 
                       const char * ts, int wl, const char * ap, int al)
{

  int nh=0;

  

  if (nh < maxn) {
    wlst[nh].word = mystrdup(ts);
    wlst[nh].allow = (1 == 0);
    nh++;
  }

  
  for (int i = 0; i < al; i++) {
    unsigned char c = (unsigned char) ap[i];
    SfxEntry * sptr = (SfxEntry *)sFlag[c];
    while (sptr) {
      char * newword = sptr->add(ts, wl);
      if (newword) {
        if (nh < maxn) {
          wlst[nh].word = newword;
          wlst[nh].allow = sptr->allowCross();
          nh++;
        } else {
          free(newword);
        }
      }
      sptr = (SfxEntry *)sptr ->getFlgNxt();
    }
  }

  int n = nh;

  
  for (int j=1;j<n ;j++)
    if (wlst[j].allow) {
      for (int k = 0; k < al; k++) {
        unsigned char c = (unsigned char) ap[k];
        PfxEntry * cptr = (PfxEntry *) pFlag[c];
        while (cptr) {
          if (cptr->allowCross()) {
            int l1 = strlen(wlst[j].word);
            char * newword = cptr->add(wlst[j].word, l1);
            if (newword) {
              if (nh < maxn) {
                wlst[nh].word = newword;
                wlst[nh].allow = cptr->allowCross();
                nh++;
              } else {
                free(newword);
              }
            }
          }
          cptr = (PfxEntry *)cptr ->getFlgNxt();
        }
      }
    }


  
  for (int m = 0; m < al; m ++) {
    unsigned char c = (unsigned char) ap[m];
    PfxEntry * ptr = (PfxEntry *) pFlag[c];
    while (ptr) {
      char * newword = ptr->add(ts, wl);
      if (newword) {
        if (nh < maxn) {
          wlst[nh].word = newword;
          wlst[nh].allow = ptr->allowCross();
          nh++;
        } else {
          free(newword);
        } 
      }
      ptr = (PfxEntry *)ptr ->getFlgNxt();
    }
  }

  return nh;
}



int AffixMgr::get_numrep()
{
  return numrep;
}


struct replentry * AffixMgr::get_reptable()
{
  if (! reptable ) return NULL;
  return reptable;
}



int AffixMgr::get_nummap()
{
  return nummap;
}


struct mapentry * AffixMgr::get_maptable()
{
  if (! maptable ) return NULL;
  return maptable;
}


char * AffixMgr::get_encoding()
{
  if (! encoding ) {
    encoding = mystrdup("ISO8859-1");
  }
  return mystrdup(encoding);
}



char * AffixMgr::get_try_string()
{
  if (! trystring ) return NULL;
  return mystrdup(trystring);
}


char * AffixMgr::get_compound()
{
  if (! compound ) return NULL;
  return compound;
}


struct hentry * AffixMgr::lookup(const char * word)
{
  if (! pHMgr) return NULL;
  return pHMgr->lookup(word);
}


bool AffixMgr::get_nosplitsugs(void)
{
  return nosplitsugs;
}


int  AffixMgr::parse_try(char * line)
{
  if (trystring) {
    fprintf(stderr,"error: duplicate TRY strings\n");
    return 1;
  }
  char * tp = line;
  char * piece;
  int i = 0;
  int np = 0;
  while ((piece=mystrsep(&tp,' '))) {
    if (*piece != '\0') {
      switch(i) {
        case 0: { np++; break; }
        case 1: { trystring = mystrdup(piece); np++; break; }
        default: break;
      }
      i++;
    }
    free(piece);
  }
  if (np != 2) {
    fprintf(stderr,"error: missing TRY information\n");
    return 1;
  } 
  return 0;
}



int  AffixMgr::parse_set(char * line)
{
  if (encoding) {
    fprintf(stderr,"error: duplicate SET strings\n");
    return 1;
  }
  char * tp = line;
  char * piece;
  int i = 0;
  int np = 0;
  while ((piece=mystrsep(&tp,' '))) {
    if (*piece != '\0') {
      switch(i) {
        case 0: { np++; break; }
        case 1: { encoding = mystrdup(piece); np++; break; }
        default: break;
      }
      i++;
    }
    free(piece);
  }
  if (np != 2) {
    fprintf(stderr,"error: missing SET information\n");
    return 1;
  } 
  return 0;
}



int  AffixMgr::parse_cpdflag(char * line)
{
  if (compound) {
    fprintf(stderr,"error: duplicate compound flags used\n");
    return 1;
  }
  char * tp = line;
  char * piece;
  int i = 0;
  int np = 0;
  while ((piece=mystrsep(&tp,' '))) {
    if (*piece != '\0') {
      switch(i) {
        case 0: { np++; break; }
        case 1: { compound = mystrdup(piece); np++; break; }
        default: break;
      }
      i++;
    }
    free(piece);
  }
  if (np != 2) {
    fprintf(stderr,"error: missing compound flag information\n");
    return 1;
  }
  return 0;
}



int  AffixMgr::parse_cpdmin(char * line)
{
  char * tp = line;
  char * piece;
  int i = 0;
  int np = 0;
  while ((piece=mystrsep(&tp,' '))) {
    if (*piece != '\0') {
      switch(i) {
        case 0: { np++; break; }
        case 1: { cpdmin = atoi(piece); np++; break; }
        default: break;
      }
      i++;
    }
    free(piece);
  }
  if (np != 2) {
    fprintf(stderr,"error: missing compound min information\n");
    return 1;
  } 
  if ((cpdmin < 1) || (cpdmin > 50)) cpdmin = 3;
  return 0;
}



int  AffixMgr::parse_reptable(char * line, FILE * af)
{
  if (numrep != 0) {
    fprintf(stderr,"error: duplicate REP tables used\n");
    return 1;
  }
  char * tp = line;
  char * piece;
  int i = 0;
  int np = 0;
  while ((piece=mystrsep(&tp,' '))) {
    if (*piece != '\0') {
      switch(i) {
        case 0: { np++; break; }
        case 1: { 
          numrep = atoi(piece);
          if (numrep < 1) {
            fprintf(stderr,"incorrect number of entries in replacement table\n");
            free(piece);
            return 1;
          }
          reptable = (replentry *) malloc(numrep * sizeof(struct replentry));
          np++;
          break;
        }
        default: break;
      }
      i++;
    }
    free(piece);
  }
  if (np != 2) {
    fprintf(stderr,"error: missing replacement table information\n");
    return 1;
  } 
 
  
  char * nl = line;
  for (int j=0; j < numrep; j++) {
    fgets(nl,MAXLNLEN,af);
    mychomp(nl);
    tp = nl;
    i = 0;
    reptable[j].pattern = NULL;
    reptable[j].replacement = NULL;
    while ((piece=mystrsep(&tp,' '))) {
      if (*piece != '\0') {
        switch(i) {
          case 0: {
            if (strncmp(piece,"REP",3) != 0) {
              fprintf(stderr,"error: replacement table is corrupt\n");
              free(piece);
              return 1;
            }
            break;
          }
          case 1: { reptable[j].pattern = mystrdup(piece); break; }
          case 2: { reptable[j].replacement = mystrdup(piece); break; }
          default: break;
        }
        i++;
      }
      free(piece);
    }
    if ((!(reptable[j].pattern)) || (!(reptable[j].replacement))) {
      fprintf(stderr,"error: replacement table is corrupt\n");
      return 1;
    }
  }
  return 0;
}




int  AffixMgr::parse_maptable(char * line, FILE * af)
{
  if (nummap != 0) {
    fprintf(stderr,"error: duplicate MAP tables used\n");
    return 1;
  }
  char * tp = line;
  char * piece;
  int i = 0;
  int np = 0;
  while ((piece=mystrsep(&tp,' '))) {
    if (*piece != '\0') {
      switch(i) {
        case 0: { np++; break; }
        case 1: { 
          nummap = atoi(piece);
          if (nummap < 1) {
            fprintf(stderr,"incorrect number of entries in map table\n");
            free(piece);
            return 1;
          }
          maptable = (mapentry *) malloc(nummap * sizeof(struct mapentry));
          np++;
          break;
        }
        default: break;
      }
      i++;
    }
    free(piece);
  }
  if (np != 2) {
    fprintf(stderr,"error: missing map table information\n");
    return 1;
  } 
 
  
  char * nl = line;
  for (int j=0; j < nummap; j++) {
    fgets(nl,MAXLNLEN,af);
    mychomp(nl);
    tp = nl;
    i = 0;
    maptable[j].set = NULL;
    maptable[j].len = 0;
    while ((piece=mystrsep(&tp,' '))) {
      if (*piece != '\0') {
        switch(i) {
          case 0: {
            if (strncmp(piece,"MAP",3) != 0) {
              fprintf(stderr,"error: map table is corrupt\n");
              free(piece);
              return 1;
            }
            break;
          }
          case 1: { maptable[j].set = mystrdup(piece); 
                    maptable[j].len = strlen(maptable[j].set);
                    break; }
          default: break;
        }
        i++;
      }
      free(piece);
    }
    if ((!(maptable[j].set)) || (!(maptable[j].len))) {
      fprintf(stderr,"error: map table is corrupt\n");
      return 1;
    }
  }
  return 0;
}




int  AffixMgr::parse_affix(char * line, const char at, FILE * af)
{
  int numents = 0;      
  char achar='\0';      
  short ff=0;
  struct affentry * ptr= NULL;
  struct affentry * nptr= NULL;

  char * tp = line;
  char * nl = line;
  char * piece;
  int i = 0;

  

  int np = 0;
  while ((piece=mystrsep(&tp,' '))) {
    if (*piece != '\0') {
      switch(i) {
        
        case 0: { np++; break; }
          
        
        case 1: { np++; achar = *piece; break; }

        
        case 2: { np++; if (*piece == 'Y') ff = XPRODUCT; break; }

        
        case 3: { 
          np++;
          numents = atoi(piece); 
          ptr = (struct affentry *) malloc(numents * sizeof(struct affentry));
          ptr->xpflg = ff;
          ptr->achar = achar;
          break;
        }

        default: break;
      }
      i++;
    }
    free(piece);
  }
  
  if (np != 4) {
    fprintf(stderr, "error: affix %c header has insufficient data in line %s\n",achar,nl);
    free(ptr);
    return 1;
  }
 
  
  nptr = ptr;

  
  for (int j=0; j < numents; j++) {
    fgets(nl,MAXLNLEN,af);
    mychomp(nl);
    tp = nl;
    i = 0;
    np = 0;

    
    while ((piece=mystrsep(&tp,' '))) {
      if (*piece != '\0') {
        switch(i) {

          
          case 0: { 
            np++;
            if (nptr != ptr) nptr->xpflg = ptr->xpflg;
            break;
          }

          
          case 1: { 
            np++;
            if (*piece != achar) {
              fprintf(stderr, "error: affix %c is corrupt near line %s\n",achar,nl);
              fprintf(stderr, "error: possible incorrect count\n");
              free(piece);
              return 1;
            }
            if (nptr != ptr) nptr->achar = ptr->achar;
            break;
          }

          
          case 2: { 
            np++;
            nptr->strip = mystrdup(piece);
            nptr->stripl = strlen(nptr->strip);
            if (strcmp(nptr->strip,"0") == 0) {
              free(nptr->strip);
              nptr->strip=mystrdup("");
              nptr->stripl = 0;
            }   
            break; 
          }

          
          case 3: { 
            np++;
            nptr->appnd = mystrdup(piece);
            nptr->appndl = strlen(nptr->appnd);
            if (strcmp(nptr->appnd,"0") == 0) {
              free(nptr->appnd);
              nptr->appnd=mystrdup("");
              nptr->appndl = 0;
            }   
            break; 
          }

          
          case 4: { np++; encodeit(nptr,piece); }

          default: break;
        }
        i++;
      }
      free(piece);
    }
    
    if (np != 5) {
      fprintf(stderr, "error: affix %c is corrupt near line %s\n",achar,nl);
      free(ptr);
      return 1;
    }
    nptr++;
  }
         
  
  
  nptr = ptr;
  for (int k = 0; k < numents; k++) {
    if (at == 'P') {
      PfxEntry * pfxptr = new PfxEntry(this,nptr);
      build_pfxtree((AffEntry *)pfxptr);
    } else {
      SfxEntry * sfxptr = new SfxEntry(this,nptr);
      build_sfxtree((AffEntry *)sfxptr); 
    }
    nptr++;
  }      
  free(ptr);
  return 0;
}
