#include "license.readme"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "hashmgr.hxx"

extern void mychomp(char * s);
extern char * mystrdup(const char *);

#ifdef __SUNPRO_CC 
using namespace std;
#endif



HashMgr::HashMgr(const char * tpath)
{
  tablesize = 0;
  tableptr = NULL;
  int ec = load_tables(tpath);
  if (ec) {
    
    fprintf(stderr,"Hash Manager Error : %d\n",ec);
    fflush(stderr);
    if (tableptr) {
      free(tableptr);
      tableptr = 0;
    }
    tablesize = 0;
  }
}


HashMgr::~HashMgr()
{
  if (tableptr) {
    
    
    for (int i=0; i < tablesize; i++) {
      struct hentry * pt = &tableptr[i];
      struct hentry * nt = NULL;
      if (pt) {
	if (pt->word) free(pt->word);
        if (pt->astr) free(pt->astr);
        pt = pt->next;
      }
      while(pt) {
        nt = pt->next;
	if (pt->word) free(pt->word);
        if (pt->astr) free(pt->astr);
        free(pt);
	pt = nt;
      }
    }
    free(tableptr);
  }
  tablesize = 0;
}





struct hentry * HashMgr::lookup(const char *word) const
{
    struct hentry * dp;
    if (tableptr) {
       dp = &tableptr[hash(word)];
       if (dp->word == NULL) return NULL;
       for (  ;  dp != NULL;  dp = dp->next) {
          if (strcmp(word,dp->word) == 0) return dp;
       }
    }
    return NULL;
}





int HashMgr::add_word(const char * word, int wl, const char * aff, int al)
{
    int i = hash(word);
    struct hentry * dp = &tableptr[i];
    struct hentry* hp;
    if (dp->word == NULL) {
       dp->wlen = wl;
       dp->alen = al;
       dp->word = mystrdup(word);
       dp->astr = mystrdup(aff);
       dp->next = NULL;
       if ((wl) && (dp->word == NULL)) return 1;
       if ((al) && (dp->astr == NULL)) return 1;
    } else {
       hp = (struct hentry *) malloc (sizeof(struct hentry));
       if (hp == NULL) return 1;
       hp->wlen = wl;
       hp->alen = al;
       hp->word = mystrdup(word);
       hp->astr = mystrdup(aff);
       hp->next = NULL;      
       while (dp->next != NULL) dp=dp->next; 
       dp->next = hp;
       if ((wl) && (hp->word == NULL)) return 1;
       if ((al) && (hp->astr == NULL)) return 1;
    }
    return 0;
}     




struct hentry * HashMgr::walk_hashtable(int &col, struct hentry * hp) const
{
  
  if ((col < 0) || (hp == NULL)) {
    col = -1;
    hp = NULL;
  }

  if (hp && hp->next != NULL) {
    hp = hp->next;
  } else {
    col++;
    hp = (col < tablesize) ? &tableptr[col] : NULL;
    
    while (hp && (hp->word == NULL)) {
        col ++;
        hp = (col < tablesize) ? &tableptr[col] : NULL;
    }
    if (col < tablesize) return hp;
    hp = NULL;
    col = -1;
  }
  return hp;
}





int HashMgr::load_tables(const char * tpath)
{
  int wl, al;
  char * ap;
  
  
  FILE * rawdict = fopen(tpath, "r");
  if (rawdict == NULL) {
    return 1;
  }

  
  char ts[MAXDELEN];
  if (!fgets(ts, MAXDELEN-1,rawdict)) {
    fclose(rawdict);
    return 2;
  }

  mychomp(ts);
  tablesize = atoi(ts);
  if (!tablesize) {
    fclose(rawdict);
    return 4; 
  }

  tablesize = tablesize + 5;
  if ((tablesize %2) == 0) tablesize++;

  
  tableptr = (struct hentry *) calloc(tablesize, sizeof(struct hentry));
  if (!tableptr) {
    fclose(rawdict);
    return 3;
  }

  
  

  while (fgets(ts,MAXDELEN-1,rawdict)) {
    mychomp(ts);
    
    ap = strchr(ts,'/');
    if (ap) {
      *ap = '\0';
      ap++;
      al = strlen(ap);
    } else {
      al = 0;
      ap = NULL;
    }

    wl = strlen(ts);

    
    if (add_word(ts,wl,ap,al)) { 
      fclose(rawdict);
      return 5;
    }

  }

  fclose(rawdict);
  return 0;
}





int HashMgr::hash(const char * word) const
{
    long  hv = 0;
    for (int i=0; i < 4  &&  *word != 0; i++)
	hv = (hv << 8) | (*word++);
    while (*word != 0) {
      ROTATE(hv,ROTATE_LEN);
      hv ^= (*word++);
    }
    return (unsigned long) hv % tablesize;
}

