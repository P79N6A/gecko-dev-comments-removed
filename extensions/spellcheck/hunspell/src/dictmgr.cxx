
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "dictmgr.hxx"
#include "csutil.hxx"

DictMgr::DictMgr(const char * dictpath, const char * etype) : numdict(0)
{
  
  pdentry = (dictentry *)malloc(MAXDICTIONARIES*sizeof(struct dictentry));
  if (pdentry) {
     if (parse_file(dictpath, etype)) {
        numdict = 0;
        
     }
  }
}


DictMgr::~DictMgr() 
{
  dictentry * pdict = NULL;
  if (pdentry) {
     pdict = pdentry;
     for (int i=0;i<numdict;i++) {
        if (pdict->lang) {
            free(pdict->lang);
            pdict->lang = NULL;
        }
        if (pdict->region) {
            free(pdict->region);
            pdict->region=NULL;
        }
        if (pdict->filename) {
            free(pdict->filename);
            pdict->filename = NULL;
        }
        pdict++;
     }
     free(pdentry);
     pdentry = NULL;
     pdict = NULL;
  }
  numdict = 0;
}



int  DictMgr::parse_file(const char * dictpath, const char * etype)
{

    int i;
    char line[MAXDICTENTRYLEN+1];
    dictentry * pdict = pdentry;

    
    FILE * dictlst;
    dictlst = myfopen(dictpath,"r");
    if (!dictlst) {
      return 1;
    }

    
    

    
    while (fgets(line,MAXDICTENTRYLEN,dictlst)) {
       mychomp(line);

       
       if (strncmp(line,etype,4) == 0) {
          if (numdict < MAXDICTIONARIES) {
             char * tp = line;
             char * piece;
             i = 0;
             while ((piece=mystrsep(&tp,' '))) {
                if (*piece != '\0') {
                    switch(i) {
                       case 0: break;
                       case 1: pdict->lang = mystrdup(piece); break;
                       case 2: if (strcmp (piece, "ANY") == 0)
                                 pdict->region = mystrdup("");
                               else
                                 pdict->region = mystrdup(piece);
                               break;
                       case 3: pdict->filename = mystrdup(piece); break;
                       default: break;
                    }
                    i++;
                }
                free(piece);
             }
             if (i == 4) {
                 numdict++;
                 pdict++;
             } else {
                 switch (i) {
                    case 3:
                       free(pdict->region);
                       pdict->region=NULL;
                       
                    case 2:
                       free(pdict->lang);
                       pdict->lang=NULL;
                    default:
                        break;
                 }
                 fprintf(stderr,"dictionary list corruption in line \"%s\"\n",line);
                 fflush(stderr);
             }
          }
       }
    }
    fclose(dictlst);
    return 0;
}


int DictMgr::get_list(dictentry ** ppentry)
{
  *ppentry = pdentry;
  return numdict;
}







char * DictMgr::mystrsep(char ** stringp, const char delim)
{
  char * rv = NULL;
  char * mp = *stringp;
  size_t n = strlen(mp);
  if (n > 0) {
     char * dp = (char *)memchr(mp,(int)((unsigned char)delim),n);
     if (dp) {
        *stringp = dp+1;
        size_t nc = dp - mp; 
        rv = (char *) malloc(nc+1);
        if (rv) {
           memcpy(rv,mp,nc);
           *(rv+nc) = '\0';
        }
     } else {
       rv = (char *) malloc(n+1);
       if (rv) {
          memcpy(rv, mp, n);
          *(rv+n) = '\0';
          *stringp = mp + n;
       }
     }
  }
  return rv;
}



char * DictMgr::mystrdup(const char * s)
{
  char * d = NULL;
  if (s) {
     int sl = strlen(s)+1;
     d = (char *) malloc(sl);
     if (d) memcpy(d,s,sl);
  }
  return d;
}



void DictMgr:: mychomp(char * s)
{
  int k = strlen(s);
  if ((k > 0) && ((*(s+k-1)=='\r') || (*(s+k-1)=='\n'))) *(s+k-1) = '\0';
  if ((k > 1) && (*(s+k-2) == '\r')) *(s+k-2) = '\0';
}

