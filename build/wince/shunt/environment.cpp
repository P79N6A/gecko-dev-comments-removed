









































#ifndef WINCE_SKIP_SHUNT_INCLUDE
#include "include/mozce_shunt.h"
#endif
#include "include/environment.h"
#include "time_conversions.h"
#include <stdlib.h>
#include "Windows.h"





typedef struct env_entry env_entry;

#define ENV_IS_STATIC 0
#define ENV_FREE_KEY 1
#define ENV_FREE_BOTH 2

struct env_entry {
  char *key;
  char *value;
  int flag;
  env_entry *next;
};

static bool env_ready = false;
static env_entry *env_head = NULL;

static env_entry *
env_find_key(const char *key)
{
  env_entry *entry = env_head;
  while (entry) {
    if (strcmp(entry->key, key) == 0)
      return entry;

    entry = entry->next;
  }

  return NULL;
}

static void
putenv_internal(char *key, char *value, int flag)
{
  env_entry *entry = env_find_key(key);
  if (entry) {
    
    
    if (entry->flag == ENV_FREE_BOTH)
      free (entry->value);
    if (entry->flag == ENV_FREE_KEY)
      free (entry->key);
  } else {
    entry = new env_entry;
    entry->next = env_head;
    env_head = entry;
  }

  entry->key = key;
  entry->value = value;
  entry->flag = flag;
}

static void
init_initial_env() {
  env_ready = true;

  putenv_internal("NSS_DEFAULT_DB_TYPE", "sql", ENV_IS_STATIC);
  putenv_internal("NSPR_FD_CACHE_SIZE_LOW", "10", ENV_IS_STATIC);
  putenv_internal("NSPR_FD_CACHE_SIZE_HIGH", "30", ENV_IS_STATIC);
  putenv_internal("XRE_PROFILE_NAME", "default", ENV_IS_STATIC);
  putenv_internal("tmp", "/Temp", ENV_IS_STATIC);
}

void
putenv_copy(const char *k, const char *v)
{
  if (!env_ready)
    init_initial_env();

  putenv_internal(strdup(k), strdup(v), ENV_FREE_BOTH);
}

int
putenv(const char *envstr)
{
  if (!env_ready)
    init_initial_env();

  char *key = strdup(envstr);
  char *value = strchr(key, '=');
  if (!value) {
    free(key);
    return -1;
  }

  *value++ = '\0';

  putenv_internal(key, value, ENV_FREE_KEY);

  return 0;
}

char *
getenv(const char* name)
{
  if (!env_ready)
    init_initial_env();

  env_entry *entry = env_find_key(name);
  if (entry && entry->value[0] != 0) {
    return entry->value;
  }

  return NULL;
}

char
GetEnvironmentVariableW(const unsigned short* lpName,
                        unsigned short* lpBuffer,
                        unsigned long nSize)
{
  char key[256];
  int rv = WideCharToMultiByte(CP_ACP, 0, lpName, -1, key, 255, NULL, NULL);
  if (rv <= 0)
    return 0;

  key[rv] = 0;
  
  char* val = getenv(key);
  
  if (!val)
    return 0;

  
  return MultiByteToWideChar(CP_ACP, 0, val, strlen(val)+1, lpBuffer, nSize);
}

char
SetEnvironmentVariableW(const unsigned short* name,
                        const unsigned short* value)
{
  char key[256];
  char val[256];
  int rv;

  rv = WideCharToMultiByte(CP_ACP, 0, name, -1, key, 255, NULL, NULL);
  if (rv < 0)
    return rv;

  key[rv] = 0;
  
  rv = WideCharToMultiByte(CP_ACP, 0, value, -1, val, 255, NULL, NULL);
  if (rv < 0)
    return rv;

  val[rv] = 0;

  putenv_copy(key, val);
  return 0;
}


unsigned int ExpandEnvironmentStringsW(const unsigned short* lpSrc,
                                       unsigned short* lpDst,
                                       unsigned int nSize)
{
  if ( NULL == lpDst )
    return 0;

  unsigned int size = 0;
  unsigned int index = 0;
  unsigned int origLen = wcslen(lpSrc);

  const unsigned short *pIn = lpSrc;
  unsigned short *pOut = lpDst;

  while ( index < origLen ) {

    if (*pIn != L'%') {  
      if ( size++ < nSize ) *pOut = *pIn, pOut++;
      index++, pIn++;
      continue;
    }

    
    int envlen = 0;
    const unsigned short *pTmp = pIn + 1;
    while ( *pTmp != L'%' && *pTmp != L' ' ) {
      envlen++, pTmp++;
      if ( origLen < index + envlen ) {    
        while ( envlen-- ) {
          if ( size++ < nSize ) *pOut = *pIn, pOut++;
          index++, pIn++;
        }
        break;
      }
    }

    if ( *pTmp == L' ' ) { 
      while ( envlen-- ) {
        if ( size++ < nSize ) *pOut = *pIn, pOut++;
        index++, pIn++;
      }
      continue;
    }

    pIn++; 
    if ( 0 == envlen ) {  
      if ( size++ < nSize ) *pOut = *pIn, pOut++;
      index += 2, pIn++;
    } else {
      
      char key[256];
      int k = WideCharToMultiByte(CP_ACP, 0, pIn, envlen, key, 255, NULL, NULL);
      key[k] = 0;
      char *pC = getenv(key);
      if ( NULL != pC ) {
        int n = MultiByteToWideChar( CP_ACP, 0, pC, -1, pOut, nSize - size );
        if ( n > 0 ) {
          size += n - 1;  
          pOut += n - 1;
        }
      }
      index += envlen + 2;
      pIn = ++pTmp;
    }
  }

  if ( size < nSize ) lpDst[size] = 0;
  return size;
}

unsigned short *
mozce_GetEnvironmentCL()
{
  env_entry *entry = env_head;
  int len = 0;
  while (entry) {
    if (entry->flag == ENV_IS_STATIC) {
      entry = entry->next;
      continue;
    }

    len += strlen(entry->key);
    len += strlen(entry->value);

    
    len += 11 + 3 + 1;

    entry = entry->next;
  }

  if (len == 0) {
    return wcsdup(L"");
  }

  wchar_t *env = (wchar_t*) malloc(sizeof(wchar_t) * (len+1));
  if (!env)
    return NULL;

  entry = env_head;

  int pos = 0;
  while (entry) {
    if (entry->flag == ENV_IS_STATIC) {
      entry = entry->next;
      continue;
    }

    if (strchr(entry->key, '"') || strchr(entry->value, '"')) {
      
      
      RETAILMSG(1, (L"Skipping environment variable with quote marks in key or value! %S -> %s\r\n", entry->key, entry->value));
      entry = entry->next;
      continue;
    }

    wcscpy (env+pos, L" --environ:\"");
    pos += 12;
    pos += MultiByteToWideChar(CP_ACP, 0, entry->key, strlen(entry->key), env+pos, len-pos);
    env[pos++] = '=';
    pos += MultiByteToWideChar(CP_ACP, 0, entry->value, strlen(entry->value), env+pos, len-pos);
    env[pos++] = '\"';

    entry = entry->next;
  } 

  env[pos] = '\0';

  return env;
  
}
