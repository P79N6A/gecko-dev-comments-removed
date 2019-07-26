
















#include <stdio.h>
#include <stdlib.h>
#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "cmemory.h"
#include "cstring.h"
#include "pkgtypes.h"
#include "putilimp.h"

const char *pkg_writeCharListWrap(FileStream *s, CharList *l, const char *delim, const char *brk, int32_t quote)
{
    int32_t ln = 0;
    char buffer[1024];
    while(l != NULL)
    {
        if(l->str)
        {
            uprv_strncpy(buffer, l->str, 1020);
            buffer[1019]=0;

            if(quote < 0) { 
                if(buffer[uprv_strlen(buffer)-1] == '"') {
                    buffer[uprv_strlen(buffer)-1] = '\0';
                }
                if(buffer[0] == '"') {
                    uprv_strcpy(buffer, buffer+1);
                }
            } else if(quote > 0) { 
                if(l->str[0] != '"') {
                    uprv_strcpy(buffer, "\"");
                    uprv_strncat(buffer, l->str,1020);
                }
                if(l->str[uprv_strlen(l->str)-1] != '"') {
                    uprv_strcat(buffer, "\"");
                }
            }
            T_FileStream_write(s, buffer, (int32_t)uprv_strlen(buffer));

            ln += (int32_t)uprv_strlen(l->str);
        }

        if(l->next && delim)
        {
            if(ln > 60 && brk) {
                ln  = 0;
                T_FileStream_write(s, brk, (int32_t)uprv_strlen(brk));
            }
            T_FileStream_write(s, delim, (int32_t)uprv_strlen(delim));
        }
        l = l->next;
    }
    return NULL;
}


const char *pkg_writeCharList(FileStream *s, CharList *l, const char *delim, int32_t quote)
{
    char buffer[1024];
    while(l != NULL)
    {
        if(l->str)
        {
            uprv_strncpy(buffer, l->str, 1023);
            buffer[1023]=0;
            if(uprv_strlen(l->str) >= 1023)
            {
                fprintf(stderr, "%s:%d: Internal error, line too long (greater than 1023 chars)\n",
                        __FILE__, __LINE__);
                exit(0);
            }
            if(quote < 0) { 
                if(buffer[uprv_strlen(buffer)-1] == '"') {
                    buffer[uprv_strlen(buffer)-1] = '\0';
                }
                if(buffer[0] == '"') {
                    uprv_strcpy(buffer, buffer+1);
                }
            } else if(quote > 0) { 
                if(l->str[0] != '"') {
                    uprv_strcpy(buffer, "\"");
                    uprv_strcat(buffer, l->str);
                }
                if(l->str[uprv_strlen(l->str)-1] != '"') {
                    uprv_strcat(buffer, "\"");
                }
            }
            T_FileStream_write(s, buffer, (int32_t)uprv_strlen(buffer));
        }

        if(l->next && delim)
        {
            T_FileStream_write(s, delim, (int32_t)uprv_strlen(delim));
        }
        l = l->next;
    }
    return NULL;
}





uint32_t pkg_countCharList(CharList *l)
{
  uint32_t c = 0;
  while(l != NULL)
  {
    c++;
    l = l->next;
  }

  return c;
}




CharList *pkg_prependToList(CharList *l, const char *str)
{
  CharList *newList;
  newList = uprv_malloc(sizeof(CharList));

  
  if(newList == NULL) {
    return NULL;
  }

  newList->str = str;
  newList->next = l;
  return newList;
}






CharList *pkg_appendToList(CharList *l, CharList** end, const char *str)
{
  CharList *endptr = NULL, *tmp;

  if(end == NULL)
  {
    end = &endptr;
  }

  
  if((*end == NULL) && (l != NULL))
  {
    tmp = l;
    while(tmp->next)
    {
      tmp = tmp->next;
    }

    *end = tmp;
  }

  
  if(l == NULL)
    {
      l = pkg_prependToList(NULL, str);
    }
  else
    {
      (*end)->next = pkg_prependToList(NULL, str);
    }

  
  if(*end)
    {
      (*end) = (*end)->next;
    }
  else
    {
      *end = l;
    }

  return l;
}

char * convertToNativePathSeparators(char *path) {
#if defined(U_MAKE_IS_NMAKE)
    char *itr;
    while ((itr = uprv_strchr(path, U_FILE_ALT_SEP_CHAR))) {
        *itr = U_FILE_SEP_CHAR;
    }
#endif
    return path;
}

CharList *pkg_appendUniqueDirToList(CharList *l, CharList** end, const char *strAlias) {
    char aBuf[1024];
    char *rPtr;
    rPtr = uprv_strrchr(strAlias, U_FILE_SEP_CHAR);
#if (U_FILE_SEP_CHAR != U_FILE_ALT_SEP_CHAR)
    {
        char *aPtr = uprv_strrchr(strAlias, U_FILE_ALT_SEP_CHAR);
        if(!rPtr || 
            (aPtr && (aPtr > rPtr)))
        { 
            rPtr = aPtr; 
        }
    }
#endif
    if(!rPtr) {
        return l; 
    }
    if((rPtr-strAlias) >= (sizeof(aBuf)/sizeof(aBuf[0]))) {
        fprintf(stderr, "## ERR: Path too long [%d chars]: %s\n", (int)sizeof(aBuf), strAlias);
        return l;
    }
    strncpy(aBuf, strAlias,(rPtr-strAlias));
    aBuf[rPtr-strAlias]=0;  
    convertToNativePathSeparators(aBuf);

    if(!pkg_listContains(l, aBuf)) {
        return pkg_appendToList(l, end, uprv_strdup(aBuf));
    } else {
        return l; 
    }
}

#if 0
static CharList *
pkg_appendFromStrings(CharList *l, CharList** end, const char *s, int32_t len)
{
  CharList *endptr = NULL;
  const char *p;
  char *t;
  const char *targ;
  if(end == NULL) {
    end = &endptr;
  }

  if(len==-1) {
    len = uprv_strlen(s);
  }
  targ = s+len;

  while(*s && s<targ) {
    while(s<targ&&isspace(*s)) s++;
    for(p=s;s<targ&&!isspace(*p);p++);
    if(p!=s) {
      t = uprv_malloc(p-s+1);
      uprv_strncpy(t,s,p-s);
      t[p-s]=0;
      l=pkg_appendToList(l,end,t);
      fprintf(stderr, " P %s\n", t);
    }
    s=p;
  }

  return l;
}
#endif





void pkg_deleteList(CharList *l)
{
  CharList *tmp;
  while(l != NULL)
  {
    uprv_free((void*)l->str);
    tmp = l;
    l = l->next;
    uprv_free(tmp);
  }
}

UBool  pkg_listContains(CharList *l, const char *str)
{
  for(;l;l=l->next){
    if(!uprv_strcmp(l->str, str)) {
      return TRUE;
    }
  }

  return FALSE;
}
