#include "license.hunspell"
#include "license.myspell"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "affentry.hxx"
#include "csutil.hxx"

#define MAXTEMPWORDLEN (MAXWORDUTF8LEN + 4)

PfxEntry::PfxEntry(AffixMgr* pmgr, affentry* dp)
    
    : pmyMgr(pmgr)
    , next(NULL)
    , nexteq(NULL)
    , nextne(NULL)
    , flgnxt(NULL)
{
  
  aflag = dp->aflag;         
  strip = dp->strip;         
  appnd = dp->appnd;         
  stripl = dp->stripl;       
  appndl = dp->appndl;       
  numconds = dp->numconds;   
  opts = dp->opts;           
  
  if (opts & aeLONGCOND) {
    memcpy(c.conds, dp->c.l.conds1, MAXCONDLEN_1);
    c.l.conds2 = dp->c.l.conds2;
  } else memcpy(c.conds, dp->c.conds, MAXCONDLEN);
  morphcode = dp->morphcode;
  contclass = dp->contclass;
  contclasslen = dp->contclasslen;
}


PfxEntry::~PfxEntry()
{
    aflag = 0;
    if (appnd) free(appnd);
    if (strip) free(strip);
    pmyMgr = NULL;
    appnd = NULL;
    strip = NULL;
    if (opts & aeLONGCOND) free(c.l.conds2);
    if (morphcode && !(opts & aeALIASM)) free(morphcode);
    if (contclass && !(opts & aeALIASF)) free(contclass);
}


char * PfxEntry::add(const char * word, int len)
{
    char tword[MAXTEMPWORDLEN];

    if ((len > stripl || (len == 0 && pmyMgr->get_fullstrip())) && 
       (len >= numconds) && test_condition(word) &&
       (!stripl || (strncmp(word, strip, stripl) == 0)) &&
       ((MAXTEMPWORDLEN) > (len + appndl - stripl))) {
    
              char * pp = tword;
              if (appndl) {
                  strncpy(tword, appnd, MAXTEMPWORDLEN-1);
                  tword[MAXTEMPWORDLEN-1] = '\0';
                  pp += appndl;
               }
               strcpy(pp, (word + stripl));
               return mystrdup(tword);
     }
     return NULL;
}

inline char * PfxEntry::nextchar(char * p) {
    if (p) {
        p++;
        if (opts & aeLONGCOND) {
            
            if (p == c.conds + MAXCONDLEN_1) return c.l.conds2;
        
        } else if (p == c.conds + MAXCONDLEN) return NULL;
	return *p ? p : NULL;
    }
    return NULL;
}

inline int PfxEntry::test_condition(const char * st)
{
    const char * pos = NULL; 
    bool neg = false;        
    bool ingroup = false;    
    if (numconds == 0) return 1;
    char * p = c.conds;
    while (1) {
      switch (*p) {
        case '\0': return 1;
        case '[': { 
                neg = false;
                ingroup = false;
                p = nextchar(p);
                pos = st; break;
            }
        case '^': { p = nextchar(p); neg = true; break; }
        case ']': { 
                if ((neg && ingroup) || (!neg && !ingroup)) return 0;
                pos = NULL;
                p = nextchar(p);
                
                if (!ingroup && *st) for (st++; (opts & aeUTF8) && (*st & 0xc0) == 0x80; st++);
                if (*st == '\0' && p) return 0; 
                break;
            }
         case '.':
            if (!pos) { 
                p = nextchar(p);
                
                for (st++; (opts & aeUTF8) && (*st & 0xc0) == 0x80; st++);
                if (*st == '\0' && p) return 0; 
                break;
            }
            
    default: {
                if (*st == *p) {
                    st++;
                    p = nextchar(p);
                    if ((opts & aeUTF8) && (*(st - 1) & 0x80)) { 
                        while (p && (*p & 0xc0) == 0x80) {       
                            if (*p != *st) {
                                if (!pos) return 0;
                                st = pos;
                                break;
                            }
                            p = nextchar(p);
                            st++;
                        }
                        if (pos && st != pos) {
                            ingroup = true;
                            while (p && *p != ']' && ((p = nextchar(p)) != NULL));
                        }
                    } else if (pos) {
                        ingroup = true;
                        while (p && *p != ']' && ((p = nextchar(p)) != NULL));
                    }
                } else if (pos) { 
                    p = nextchar(p);
                } else return 0;
            }
      }
      if (!p) return 1;
    }
}


struct hentry * PfxEntry::checkword(const char * word, int len, char in_compound, const FLAG needflag)
{
    int                 tmpl;   
    struct hentry *     he;     
    char                tmpword[MAXTEMPWORDLEN];

    
    
    
    

     tmpl = len - appndl;

     if (tmpl > 0 || (tmpl == 0 && pmyMgr->get_fullstrip())) {

            
            

            if (stripl) {
                strncpy(tmpword, strip, MAXTEMPWORDLEN-1);
                tmpword[MAXTEMPWORDLEN-1] = '\0';
            }
            strcpy ((tmpword + stripl), (word + appndl));

            
            
            
            

            
            

            if (test_condition(tmpword)) {
                tmpl += stripl;
                if ((he = pmyMgr->lookup(tmpword)) != NULL) {
                   do {
                      if (TESTAFF(he->astr, aflag, he->alen) &&
                        
                        ! TESTAFF(contclass, pmyMgr->get_needaffix(), contclasslen) &&
                        
                        ((!needflag) || TESTAFF(he->astr, needflag, he->alen) ||
                         (contclass && TESTAFF(contclass, needflag, contclasslen))))
                            return he;
                      he = he->next_homonym; 
                   } while (he);
                }

                
                
                

                
                if ((opts & aeXPRODUCT)) {
                   he = pmyMgr->suffix_check(tmpword, tmpl, aeXPRODUCT, this, NULL,
                        0, NULL, FLAG_NULL, needflag, in_compound);
                   if (he) return he;
                }
            }
     }
    return NULL;
}


struct hentry * PfxEntry::check_twosfx(const char * word, int len,
    char in_compound, const FLAG needflag)
{
    int                 tmpl;   
    struct hentry *     he;     
    char                tmpword[MAXTEMPWORDLEN];

    
    
    
    

     tmpl = len - appndl;

     if ((tmpl > 0 || (tmpl == 0 && pmyMgr->get_fullstrip())) &&
        (tmpl + stripl >= numconds)) {

            
            

            if (stripl) {
                strncpy(tmpword, strip, MAXTEMPWORDLEN-1);
                tmpword[MAXTEMPWORDLEN-1] = '\0';
            }
            strcpy ((tmpword + stripl), (word + appndl));

            
            
            
            

            
            

            if (test_condition(tmpword)) {
                tmpl += stripl;

                
                
                

                if ((opts & aeXPRODUCT) && (in_compound != IN_CPD_BEGIN)) {
                   he = pmyMgr->suffix_check_twosfx(tmpword, tmpl, aeXPRODUCT, this, needflag);
                   if (he) return he;
                }
            }
     }
    return NULL;
}


char * PfxEntry::check_twosfx_morph(const char * word, int len,
         char in_compound, const FLAG needflag)
{
    int                 tmpl;   
    char                tmpword[MAXTEMPWORDLEN];

    
    
    
    

     tmpl = len - appndl;

     if ((tmpl > 0 || (tmpl == 0 && pmyMgr->get_fullstrip())) &&
        (tmpl + stripl >= numconds)) {

            
            

            if (stripl) {
                strncpy(tmpword, strip, MAXTEMPWORDLEN-1);
                tmpword[MAXTEMPWORDLEN-1] = '\0';
            }
            strcpy ((tmpword + stripl), (word + appndl));

            
            
            
            

            
            

            if (test_condition(tmpword)) {
                tmpl += stripl;

                
                
                

                if ((opts & aeXPRODUCT) && (in_compound != IN_CPD_BEGIN)) {
                    return pmyMgr->suffix_check_twosfx_morph(tmpword, tmpl,
                             aeXPRODUCT, this, needflag);
                }
            }
     }
    return NULL;
}


char * PfxEntry::check_morph(const char * word, int len, char in_compound, const FLAG needflag)
{
    int                 tmpl;   
    struct hentry *     he;     
    char                tmpword[MAXTEMPWORDLEN];
    char                result[MAXLNLEN];
    char * st;

    *result = '\0';

    
    
    
    

     tmpl = len - appndl;

     if ((tmpl > 0 || (tmpl == 0 && pmyMgr->get_fullstrip())) &&
        (tmpl + stripl >= numconds)) {

            
            

            if (stripl) {
                strncpy(tmpword, strip, MAXTEMPWORDLEN-1);
                tmpword[MAXTEMPWORDLEN-1] = '\0';
            }
            strcpy ((tmpword + stripl), (word + appndl));

            
            
            
            

            
            

            if (test_condition(tmpword)) {
                tmpl += stripl;
                if ((he = pmyMgr->lookup(tmpword)) != NULL) {
                    do {
                      if (TESTAFF(he->astr, aflag, he->alen) &&
                        
                        ! TESTAFF(contclass, pmyMgr->get_needaffix(), contclasslen) &&
                        
                        ((!needflag) || TESTAFF(he->astr, needflag, he->alen) ||
                         (contclass && TESTAFF(contclass, needflag, contclasslen)))) {
                            if (morphcode) {
                                mystrcat(result, " ", MAXLNLEN);
                                mystrcat(result, morphcode, MAXLNLEN);
                            } else mystrcat(result,getKey(), MAXLNLEN);
                            if (!HENTRY_FIND(he, MORPH_STEM)) {
                                mystrcat(result, " ", MAXLNLEN);
                                mystrcat(result, MORPH_STEM, MAXLNLEN);
                                mystrcat(result, HENTRY_WORD(he), MAXLNLEN);
                            }
                            
                            if (HENTRY_DATA(he)) {
                                mystrcat(result, " ", MAXLNLEN);
                                mystrcat(result, HENTRY_DATA2(he), MAXLNLEN);
                            } else {
                                
                                char * flag = pmyMgr->encode_flag(getFlag());
                                mystrcat(result, " ", MAXLNLEN);
                                mystrcat(result, MORPH_FLAG, MAXLNLEN);
                                mystrcat(result, flag, MAXLNLEN);
                                free(flag);
                            }
                            mystrcat(result, "\n", MAXLNLEN);
                      }
                      he = he->next_homonym;
                    } while (he);
                }

                
                
                

                if ((opts & aeXPRODUCT) && (in_compound != IN_CPD_BEGIN)) {
                   st = pmyMgr->suffix_check_morph(tmpword, tmpl, aeXPRODUCT, this,
                     FLAG_NULL, needflag);
                   if (st) {
                        mystrcat(result, st, MAXLNLEN);
                        free(st);
                   }
                }
            }
     }
    
    if (*result) return mystrdup(result);
    return NULL;
}

SfxEntry::SfxEntry(AffixMgr * pmgr, affentry* dp)
    : pmyMgr(pmgr) 
    , next(NULL)
    , nexteq(NULL)
    , nextne(NULL)
    , flgnxt(NULL)
    , l_morph(NULL)
    , r_morph(NULL)
    , eq_morph(NULL)
{
  
  aflag = dp->aflag;         
  strip = dp->strip;         
  appnd = dp->appnd;         
  stripl = dp->stripl;       
  appndl = dp->appndl;       
  numconds = dp->numconds;   
  opts = dp->opts;           

  
  if (opts & aeLONGCOND) {
    memcpy(c.l.conds1, dp->c.l.conds1, MAXCONDLEN_1);
    c.l.conds2 = dp->c.l.conds2;
  } else memcpy(c.conds, dp->c.conds, MAXCONDLEN);
  rappnd = myrevstrdup(appnd);
  morphcode = dp->morphcode;
  contclass = dp->contclass;
  contclasslen = dp->contclasslen;
}


SfxEntry::~SfxEntry()
{
    aflag = 0;
    if (appnd) free(appnd);
    if (rappnd) free(rappnd);
    if (strip) free(strip);
    pmyMgr = NULL;
    appnd = NULL;
    strip = NULL;
    if (opts & aeLONGCOND) free(c.l.conds2);
    if (morphcode && !(opts & aeALIASM)) free(morphcode);
    if (contclass && !(opts & aeALIASF)) free(contclass);
}


char * SfxEntry::add(const char * word, int len)
{
    char tword[MAXTEMPWORDLEN];

     
     if ((len > stripl || (len == 0 && pmyMgr->get_fullstrip())) &&
        (len >= numconds) && test_condition(word + len, word) &&
        (!stripl || (strcmp(word + len - stripl, strip) == 0)) &&
        ((MAXTEMPWORDLEN) > (len + appndl - stripl))) {
              
              strncpy(tword, word, MAXTEMPWORDLEN-1);
              tword[MAXTEMPWORDLEN-1] = '\0';
              if (appndl) {
                  strcpy(tword + len - stripl, appnd);
              } else {
                  *(tword + len - stripl) = '\0';
              }
              return mystrdup(tword);
     }
     return NULL;
}

inline char * SfxEntry::nextchar(char * p) {
    if (p) {
	p++;
	if (opts & aeLONGCOND) {
    	    
    	    if (p == c.l.conds1 + MAXCONDLEN_1) return c.l.conds2;
	
	} else if (p == c.conds + MAXCONDLEN) return NULL;
	return *p ? p : NULL;
    }
    return NULL;
}

inline int SfxEntry::test_condition(const char * st, const char * beg)
{
    const char * pos = NULL;    
    bool neg = false;           
    bool ingroup = false;       
    if (numconds == 0) return 1;
    char * p = c.conds;
    st--;
    int i = 1;
    while (1) {
      switch (*p) {
        case '\0':
            return 1;
        case '[':
            p = nextchar(p);
            pos = st;
            break;
        case '^':
            p = nextchar(p);
            neg = true;
            break;
        case ']':
            if (!neg && !ingroup)
              return 0;
            i++;
            
            if (!ingroup)
            {
                for (; (opts & aeUTF8) && (st >= beg) && (*st & 0xc0) == 0x80; st--);
                st--;
            }                    
            pos = NULL;
            neg = false;
            ingroup = false;
            p = nextchar(p);
            if (st < beg && p)
                return 0; 
            break;
        case '.':
            if (!pos)
            {
                
                p = nextchar(p);
                
                for (st--; (opts & aeUTF8) && (st >= beg) && (*st & 0xc0) == 0x80; st--);
                if (st < beg) { 
		    if (p) return 0; else return 1;
		}
                if ((opts & aeUTF8) && (*st & 0x80)) { 
                    st--;
                    if (st < beg) { 
			if (p) return 0; else return 1;
		    }
                }
                break;
            }
            
    default: {
                if (*st == *p) {
                    p = nextchar(p);
                    if ((opts & aeUTF8) && (*st & 0x80)) {
                        st--;
                        while (p && (st >= beg)) {
                            if (*p != *st) {
                                if (!pos) return 0;
                                st = pos;
                                break;
                            }
                            
                            if ((*p & 0xc0) != 0x80) break;
                            p = nextchar(p);
                            st--;
                        }
                        if (pos && st != pos) {
                            if (neg) return 0;
                            else if (i == numconds) return 1;
                            ingroup = true;
                            while (p && *p != ']' && ((p = nextchar(p)) != NULL));
			    st--;
                        }
                        if (p && *p != ']') p = nextchar(p);
                    } else if (pos) {
                        if (neg) return 0;
                        else if (i == numconds) return 1;
                        ingroup = true;
			while (p && *p != ']' && ((p = nextchar(p)) != NULL))
                           ;

                        st--;
                    }
                    if (!pos) {
                        i++;
                        st--;
                    }
                    if (st < beg && p && *p != ']') return 0; 
                } else if (pos) { 
                    p = nextchar(p);
                } else return 0;
            }
      }
      if (!p) return 1;
    }
}


struct hentry * SfxEntry::checkword(const char * word, int len, int optflags,
    PfxEntry* ppfx, char ** wlst, int maxSug, int * ns, const FLAG cclass, const FLAG needflag,
    const FLAG badflag)
{
    int                 tmpl;            
    struct hentry *     he;              
    unsigned char *     cp;
    char                tmpword[MAXTEMPWORDLEN];
    PfxEntry* ep = ppfx;

    
    

    if (((optflags & aeXPRODUCT) != 0) && ((opts & aeXPRODUCT) == 0))
        return NULL;

    
    
    
    

    tmpl = len - appndl;
    
    

    if ((tmpl > 0 || (tmpl == 0 && pmyMgr->get_fullstrip())) &&
        (tmpl + stripl >= numconds)) {

            
            
            

            strncpy (tmpword, word, MAXTEMPWORDLEN-1);
            tmpword[MAXTEMPWORDLEN-1] = '\0';
            cp = (unsigned char *)(tmpword + tmpl);
            if (stripl) {
                strcpy ((char *)cp, strip);
                tmpl += stripl;
                cp = (unsigned char *)(tmpword + tmpl);
            } else *cp = '\0';

            
            
            
            

            
            

            if (test_condition((char *) cp, (char *) tmpword)) {

#ifdef SZOSZABLYA_POSSIBLE_ROOTS
                fprintf(stdout,"%s %s %c\n", word, tmpword, aflag);
#endif
                if ((he = pmyMgr->lookup(tmpword)) != NULL) {
                    do {
                        
                        if ((TESTAFF(he->astr, aflag, he->alen) || (ep && ep->getCont() &&
                                    TESTAFF(ep->getCont(), aflag, ep->getContLen()))) &&
                            (((optflags & aeXPRODUCT) == 0) ||
                            (ep && TESTAFF(he->astr, ep->getFlag(), he->alen)) ||
                             
                            ((contclass) && (ep && TESTAFF(contclass, ep->getFlag(), contclasslen)))
                            ) &&
                            
                            ((!cclass) ||
                                ((contclass) && TESTAFF(contclass, cclass, contclasslen))
                            ) &&
                            
                            (!badflag || !TESTAFF(he->astr, badflag, he->alen)
                            ) &&
                            
                            ((!needflag) ||
                              (TESTAFF(he->astr, needflag, he->alen) ||
                              ((contclass) && TESTAFF(contclass, needflag, contclasslen)))
                            )
                        ) return he;
                        he = he->next_homonym; 
                    } while (he);

                
                
                
                } else if (wlst && (*ns < maxSug)) {
                    int cwrd = 1;
                    for (int k=0; k < *ns; k++)
                        if (strcmp(tmpword, wlst[k]) == 0) {
                           cwrd = 0;
                           break;
                        }
                    if (cwrd) {
                        wlst[*ns] = mystrdup(tmpword);
                        if (wlst[*ns] == NULL) {
                            for (int j=0; j<*ns; j++) free(wlst[j]);
                            *ns = -1;
                            return NULL;
                        }
                        (*ns)++;
                    }
                }
            }
    }
    return NULL;
}


struct hentry * SfxEntry::check_twosfx(const char * word, int len, int optflags,
    PfxEntry* ppfx, const FLAG needflag)
{
    int                 tmpl;            
    struct hentry *     he;              
    unsigned char *     cp;
    char                tmpword[MAXTEMPWORDLEN];
    PfxEntry* ep = ppfx;


    
    

    if ((optflags & aeXPRODUCT) != 0 &&  (opts & aeXPRODUCT) == 0)
        return NULL;

    
    
    
    

    tmpl = len - appndl;

    if ((tmpl > 0 || (tmpl == 0 && pmyMgr->get_fullstrip())) &&
       (tmpl + stripl >= numconds)) {

            
            
            

            strncpy(tmpword, word, MAXTEMPWORDLEN-1);
            tmpword[MAXTEMPWORDLEN-1] = '\0';
            cp = (unsigned char *)(tmpword + tmpl);
            if (stripl) {
                strcpy ((char *)cp, strip);
                tmpl += stripl;
                cp = (unsigned char *)(tmpword + tmpl);
            } else *cp = '\0';

            
            
            
            

            

            if (test_condition((char *) cp, (char *) tmpword)) {
                if (ppfx) {
                    
                    if ((contclass) && TESTAFF(contclass, ep->getFlag(), contclasslen))
                        he = pmyMgr->suffix_check(tmpword, tmpl, 0, NULL, NULL, 0, NULL, (FLAG) aflag, needflag);
                    else
                        he = pmyMgr->suffix_check(tmpword, tmpl, optflags, ppfx, NULL, 0, NULL, (FLAG) aflag, needflag);
                } else {
                    he = pmyMgr->suffix_check(tmpword, tmpl, 0, NULL, NULL, 0, NULL, (FLAG) aflag, needflag);
                }
                if (he) return he;
            }
    }
    return NULL;
}


char * SfxEntry::check_twosfx_morph(const char * word, int len, int optflags,
    PfxEntry* ppfx, const FLAG needflag)
{
    int                 tmpl;            
    unsigned char *     cp;
    char                tmpword[MAXTEMPWORDLEN];
    PfxEntry* ep = ppfx;
    char * st;

    char result[MAXLNLEN];

    *result = '\0';

    
    

    if ((optflags & aeXPRODUCT) != 0 &&  (opts & aeXPRODUCT) == 0)
        return NULL;

    
    
    
    

    tmpl = len - appndl;

    if ((tmpl > 0 || (tmpl == 0 && pmyMgr->get_fullstrip())) &&
       (tmpl + stripl >= numconds)) {

            
            
            

            strncpy(tmpword, word, MAXTEMPWORDLEN-1);
            tmpword[MAXTEMPWORDLEN-1] = '\0';
            cp = (unsigned char *)(tmpword + tmpl);
            if (stripl) {
                strcpy ((char *)cp, strip);
                tmpl += stripl;
                cp = (unsigned char *)(tmpword + tmpl);
            } else *cp = '\0';

            
            
            
            

            

            if (test_condition((char *) cp, (char *) tmpword)) {
                if (ppfx) {
                    
                    if ((contclass) && TESTAFF(contclass, ep->getFlag(), contclasslen)) {
                        st = pmyMgr->suffix_check_morph(tmpword, tmpl, 0, NULL, aflag, needflag);
                        if (st) {
                            if (ppfx->getMorph()) {
                                mystrcat(result, ppfx->getMorph(), MAXLNLEN);
                                mystrcat(result, " ", MAXLNLEN);
                            }
                            mystrcat(result,st, MAXLNLEN);
                            free(st);
                            mychomp(result);
                        }
                    } else {
                        st = pmyMgr->suffix_check_morph(tmpword, tmpl, optflags, ppfx, aflag, needflag);
                        if (st) {
                            mystrcat(result, st, MAXLNLEN);
                            free(st);
                            mychomp(result);
                        }
                    }
                } else {
                        st = pmyMgr->suffix_check_morph(tmpword, tmpl, 0, NULL, aflag, needflag);
                        if (st) {
                            mystrcat(result, st, MAXLNLEN);
                            free(st);
                            mychomp(result);
                        }
                }
                if (*result) return mystrdup(result);
            }
    }
    return NULL;
}


struct hentry * SfxEntry::get_next_homonym(struct hentry * he, int optflags, PfxEntry* ppfx,
    const FLAG cclass, const FLAG needflag)
{
    PfxEntry* ep = ppfx;
    FLAG eFlag = ep ? ep->getFlag() : FLAG_NULL;

    while (he->next_homonym) {
        he = he->next_homonym;
        if ((TESTAFF(he->astr, aflag, he->alen) || (ep && ep->getCont() && TESTAFF(ep->getCont(), aflag, ep->getContLen()))) &&
                            ((optflags & aeXPRODUCT) == 0 ||
                            TESTAFF(he->astr, eFlag, he->alen) ||
                             
                            ((contclass) && TESTAFF(contclass, eFlag, contclasslen))
                            ) &&
                            
                            ((!cclass) ||
                                ((contclass) && TESTAFF(contclass, cclass, contclasslen))
                            ) &&
                            
                            ((!needflag) ||
                              (TESTAFF(he->astr, needflag, he->alen) ||
                              ((contclass) && TESTAFF(contclass, needflag, contclasslen)))
                            )
                        ) return he;
    }
    return NULL;
}


#if 0

Appendix:  Understanding Affix Code


An affix is either a  prefix or a suffix attached to root words to make 
other words.

Basically a Prefix or a Suffix is set of AffEntry objects
which store information about the prefix or suffix along 
with supporting routines to check if a word has a particular 
prefix or suffix or a combination.

The structure affentry is defined as follows:

struct affentry
{
   unsigned short aflag;    
   char * strip;            
   char * appnd;            
   unsigned char stripl;    
   unsigned char appndl;    
   char numconds;           
   char opts;               
   char   conds[SETSIZE];   
};


Here is a suffix borrowed from the en_US.aff file.  This file 
is whitespace delimited.

SFX D Y 4 
SFX D   0     e          d
SFX D   y     ied        [^aeiou]y
SFX D   0     ed         [^ey]
SFX D   0     ed         [aeiou]y

This information can be interpreted as follows:

In the first line has 4 fields

Field
-----
1     SFX - indicates this is a suffix
2     D   - is the name of the character flag which represents this suffix
3     Y   - indicates it can be combined with prefixes (cross product)
4     4   - indicates that sequence of 4 affentry structures are needed to
               properly store the affix information

The remaining lines describe the unique information for the 4 SfxEntry 
objects that make up this affix.  Each line can be interpreted
as follows: (note fields 1 and 2 are as a check against line 1 info)

Field
-----
1     SFX         - indicates this is a suffix
2     D           - is the name of the character flag for this affix
3     y           - the string of chars to strip off before adding affix
                         (a 0 here indicates the NULL string)
4     ied         - the string of affix characters to add
5     [^aeiou]y   - the conditions which must be met before the affix
                    can be applied

Field 5 is interesting.  Since this is a suffix, field 5 tells us that
there are 2 conditions that must be met.  The first condition is that 
the next to the last character in the word must *NOT* be any of the 
following "a", "e", "i", "o" or "u".  The second condition is that
the last character of the word must end in "y".

So how can we encode this information concisely and be able to 
test for both conditions in a fast manner?  The answer is found
but studying the wonderful ispell code of Geoff Kuenning, et.al. 
(now available under a normal BSD license).

If we set up a conds array of 256 bytes indexed (0 to 255) and access it
using a character (cast to an unsigned char) of a string, we have 8 bits
of information we can store about that character.  Specifically we
could use each bit to say if that character is allowed in any of the 
last (or first for prefixes) 8 characters of the word.

Basically, each character at one end of the word (up to the number 
of conditions) is used to index into the conds array and the resulting 
value found there says whether the that character is valid for a 
specific character position in the word.  

For prefixes, it does this by setting bit 0 if that char is valid 
in the first position, bit 1 if valid in the second position, and so on. 

If a bit is not set, then that char is not valid for that postion in the
word.

If working with suffixes bit 0 is used for the character closest 
to the front, bit 1 for the next character towards the end, ..., 
with bit numconds-1 representing the last char at the end of the string. 

Note: since entries in the conds[] are 8 bits, only 8 conditions 
(read that only 8 character positions) can be examined at one
end of a word (the beginning for prefixes and the end for suffixes.

So to make this clearer, lets encode the conds array values for the 
first two affentries for the suffix D described earlier.


  For the first affentry:    
     numconds = 1             (only examine the last character)

     conds['e'] =  (1 << 0)   (the word must end in an E)
     all others are all 0

  For the second affentry:
     numconds = 2             (only examine the last two characters)     

     conds[X] = conds[X] | (1 << 0)     (aeiou are not allowed)
         where X is all characters *but* a, e, i, o, or u
         

     conds['y'] = (1 << 1)     (the last char must be a y)
     all other bits for all other entries in the conds array are zero


#endif

