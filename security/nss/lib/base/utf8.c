



#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: utf8.c,v $ $Revision: 1.8 $ $Date: 2012/04/25 14:49:26 $";
#endif 








#ifndef BASE_H
#include "base.h"
#endif 

#include "plstr.h"























NSS_IMPLEMENT PRBool
nssUTF8_CaseIgnoreMatch
(
  const NSSUTF8 *a,
  const NSSUTF8 *b,
  PRStatus *statusOpt
)
{
#ifdef NSSDEBUG
  if( ((const NSSUTF8 *)NULL == a) ||
      ((const NSSUTF8 *)NULL == b) ) {
    nss_SetError(NSS_ERROR_INVALID_POINTER);
    if( (PRStatus *)NULL != statusOpt ) {
      *statusOpt = PR_FAILURE;
    }
    return PR_FALSE;
  }
#endif 

  if( (PRStatus *)NULL != statusOpt ) {
    *statusOpt = PR_SUCCESS;
  }

  




  if( 0 == PL_strcasecmp((const char *)a, (const char *)b) ) {
    return PR_TRUE;
  } else {
    return PR_FALSE;
  }
}























NSS_IMPLEMENT PRBool
nssUTF8_PrintableMatch
(
  const NSSUTF8 *a,
  const NSSUTF8 *b,
  PRStatus *statusOpt
)
{
  PRUint8 *c;
  PRUint8 *d;

#ifdef NSSDEBUG
  if( ((const NSSUTF8 *)NULL == a) ||
      ((const NSSUTF8 *)NULL == b) ) {
    nss_SetError(NSS_ERROR_INVALID_POINTER);
    if( (PRStatus *)NULL != statusOpt ) {
      *statusOpt = PR_FAILURE;
    }
    return PR_FALSE;
  }
#endif 

  if( (PRStatus *)NULL != statusOpt ) {
    *statusOpt = PR_SUCCESS;
  }

  c = (PRUint8 *)a;
  d = (PRUint8 *)b;

  while( ' ' == *c ) {
    c++;
  }

  while( ' ' == *d ) {
    d++;
  }

  while( ('\0' != *c) && ('\0' != *d) ) {
    PRUint8 e, f;

    e = *c;
    f = *d;
    
    if( ('a' <= e) && (e <= 'z') ) {
      e -= ('a' - 'A');
    }

    if( ('a' <= f) && (f <= 'z') ) {
      f -= ('a' - 'A');
    }

    if( e != f ) {
      return PR_FALSE;
    }

    c++;
    d++;

    if( ' ' == *c ) {
      while( ' ' == *c ) {
        c++;
      }
      c--;
    }

    if( ' ' == *d ) {
      while( ' ' == *d ) {
        d++;
      }
      d--;
    }
  }

  while( ' ' == *c ) {
    c++;
  }

  while( ' ' == *d ) {
    d++;
  }

  if( *c == *d ) {
    
    return PR_TRUE;
  } else {
    return PR_FALSE;
  }
}


















NSS_IMPLEMENT NSSUTF8 *
nssUTF8_Duplicate
(
  const NSSUTF8 *s,
  NSSArena *arenaOpt
)
{
  NSSUTF8 *rv;
  PRUint32 len;

#ifdef NSSDEBUG
  if( (const NSSUTF8 *)NULL == s ) {
    nss_SetError(NSS_ERROR_INVALID_POINTER);
    return (NSSUTF8 *)NULL;
  }

  if( (NSSArena *)NULL != arenaOpt ) {
    if( PR_SUCCESS != nssArena_verifyPointer(arenaOpt) ) {
      return (NSSUTF8 *)NULL;
    }
  }
#endif 

  len = PL_strlen((const char *)s);
#ifdef PEDANTIC
  if( '\0' != ((const char *)s)[ len ] ) {
    
    nss_SetError(NSS_ERROR_NO_MEMORY);
    return (NSSUTF8 *)NULL;
  }
#endif 
  len++; 

  rv = nss_ZAlloc(arenaOpt, len);
  if( (void *)NULL == rv ) {
    return (NSSUTF8 *)NULL;
  }

  (void)nsslibc_memcpy(rv, s, len);
  return rv;
}

















NSS_IMPLEMENT PRUint32
nssUTF8_Size
(
  const NSSUTF8 *s,
  PRStatus *statusOpt
)
{
  PRUint32 sv;

#ifdef NSSDEBUG
  if( (const NSSUTF8 *)NULL == s ) {
    nss_SetError(NSS_ERROR_INVALID_POINTER);
    if( (PRStatus *)NULL != statusOpt ) {
      *statusOpt = PR_FAILURE;
    }
    return 0;
  }
#endif 

  sv = PL_strlen((const char *)s) + 1;
#ifdef PEDANTIC
  if( '\0' != ((const char *)s)[ sv-1 ] ) {
    
    nss_SetError(NSS_ERROR_VALUE_TOO_LARGE);
    if( (PRStatus *)NULL != statusOpt ) {
      *statusOpt = PR_FAILURE;
    }
    return 0;
  }
#endif 

  if( (PRStatus *)NULL != statusOpt ) {
    *statusOpt = PR_SUCCESS;
  }

  return sv;
}


















NSS_IMPLEMENT PRUint32
nssUTF8_Length
(
  const NSSUTF8 *s,
  PRStatus *statusOpt
)
{
  PRUint32 l = 0;
  const PRUint8 *c = (const PRUint8 *)s;

#ifdef NSSDEBUG
  if( (const NSSUTF8 *)NULL == s ) {
    nss_SetError(NSS_ERROR_INVALID_POINTER);
    goto loser;
  }
#endif 

  









  

  while( 0 != *c ) {
    PRUint32 incr;
    if( (*c & 0x80) == 0 ) {
      incr = 1;
    } else if( (*c & 0xE0) == 0xC0 ) {
      incr = 2;
    } else if( (*c & 0xF0) == 0xE0 ) {
      incr = 3;
    } else if( (*c & 0xF8) == 0xF0 ) {
      incr = 4;
    } else if( (*c & 0xFC) == 0xF8 ) {
      incr = 5;
    } else if( (*c & 0xFE) == 0xFC ) {
      incr = 6;
    } else {
      nss_SetError(NSS_ERROR_INVALID_STRING);
      goto loser;
    }

    l += incr;

#ifdef PEDANTIC
    if( l < incr ) {
      
      nss_SetError(NSS_ERROR_VALUE_TOO_LARGE);
      goto loser;
    }

    {
      PRUint8 *d;
      for( d = &c[1]; d < &c[incr]; d++ ) {
        if( (*d & 0xC0) != 0xF0 ) {
          nss_SetError(NSS_ERROR_INVALID_STRING);
          goto loser;
        }
      }
    }
#endif 

    c += incr;
  }

  if( (PRStatus *)NULL != statusOpt ) {
    *statusOpt = PR_SUCCESS;
  }

  return l;

 loser:
  if( (PRStatus *)NULL != statusOpt ) {
    *statusOpt = PR_FAILURE;
  }

  return 0;
}

























extern const NSSError NSS_ERROR_INTERNAL_ERROR; 

NSS_IMPLEMENT NSSUTF8 *
nssUTF8_Create
(
  NSSArena *arenaOpt,
  nssStringType type,
  const void *inputString,
  PRUint32 size 
)
{
  NSSUTF8 *rv = NULL;

#ifdef NSSDEBUG
  if( (NSSArena *)NULL != arenaOpt ) {
    if( PR_SUCCESS != nssArena_verifyPointer(arenaOpt) ) {
      return (NSSUTF8 *)NULL;
    }
  }

  if( (const void *)NULL == inputString ) {
    nss_SetError(NSS_ERROR_INVALID_POINTER);
    return (NSSUTF8 *)NULL;
  }
#endif 

  switch( type ) {
  case nssStringType_DirectoryString:
    
    nss_SetError(NSS_ERROR_UNSUPPORTED_TYPE);
    break;
  case nssStringType_TeletexString:
    










    nss_SetError(NSS_ERROR_INTERNAL_ERROR); 
    break;
  case nssStringType_PrintableString:
    





    if( 0 == size ) {
      rv = nssUTF8_Duplicate((const NSSUTF8 *)inputString, arenaOpt);
    } else {
      rv = nss_ZAlloc(arenaOpt, size+1);
      if( (NSSUTF8 *)NULL == rv ) {
        return (NSSUTF8 *)NULL;
      }

      (void)nsslibc_memcpy(rv, inputString, size);
    }

    break;
  case nssStringType_UniversalString:
    
    nss_SetError(NSS_ERROR_INTERNAL_ERROR); 
    break;
  case nssStringType_BMPString:
    
    nss_SetError(NSS_ERROR_INTERNAL_ERROR); 
    break;
  case nssStringType_UTF8String:
    if( 0 == size ) {
      rv = nssUTF8_Duplicate((const NSSUTF8 *)inputString, arenaOpt);
    } else {
      rv = nss_ZAlloc(arenaOpt, size+1);
      if( (NSSUTF8 *)NULL == rv ) {
        return (NSSUTF8 *)NULL;
      }

      (void)nsslibc_memcpy(rv, inputString, size);
    }

    break;
  case nssStringType_PHGString:
    




    nss_SetError(NSS_ERROR_INTERNAL_ERROR); 
    break;
  case nssStringType_GeneralString:
    nss_SetError(NSS_ERROR_INTERNAL_ERROR); 
    break;
  default:
    nss_SetError(NSS_ERROR_UNSUPPORTED_TYPE);
    break;
  }

  return rv;
}

NSS_IMPLEMENT NSSItem *
nssUTF8_GetEncoding
(
  NSSArena *arenaOpt,
  NSSItem *rvOpt,
  nssStringType type,
  NSSUTF8 *string
)
{
  NSSItem *rv = (NSSItem *)NULL;
  PRStatus status = PR_SUCCESS;

#ifdef NSSDEBUG
  if( (NSSArena *)NULL != arenaOpt ) {
    if( PR_SUCCESS != nssArena_verifyPointer(arenaOpt) ) {
      return (NSSItem *)NULL;
    }
  }

  if( (NSSUTF8 *)NULL == string ) {
    nss_SetError(NSS_ERROR_INVALID_POINTER);
    return (NSSItem *)NULL;
  }
#endif 

  switch( type ) {
  case nssStringType_DirectoryString:
    nss_SetError(NSS_ERROR_INTERNAL_ERROR); 
    break;
  case nssStringType_TeletexString:
    nss_SetError(NSS_ERROR_INTERNAL_ERROR); 
    break;
  case nssStringType_PrintableString:
    nss_SetError(NSS_ERROR_INTERNAL_ERROR); 
    break;
  case nssStringType_UniversalString:
    nss_SetError(NSS_ERROR_INTERNAL_ERROR); 
    break;
  case nssStringType_BMPString:
    nss_SetError(NSS_ERROR_INTERNAL_ERROR); 
    break;
  case nssStringType_UTF8String:
    {
      NSSUTF8 *dup = nssUTF8_Duplicate(string, arenaOpt);
      if( (NSSUTF8 *)NULL == dup ) {
        return (NSSItem *)NULL;
      }

      if( (NSSItem *)NULL == rvOpt ) {
        rv = nss_ZNEW(arenaOpt, NSSItem);
        if( (NSSItem *)NULL == rv ) {
          (void)nss_ZFreeIf(dup);
          return (NSSItem *)NULL;
        }
      } else {
        rv = rvOpt;
      }

      rv->data = dup;
      dup = (NSSUTF8 *)NULL;
      rv->size = nssUTF8_Size(rv->data, &status);
      if( (0 == rv->size) && (PR_SUCCESS != status) ) {
        if( (NSSItem *)NULL == rvOpt ) {
          (void)nss_ZFreeIf(rv);
        }
        return (NSSItem *)NULL;
      }
    }
    break;
  case nssStringType_PHGString:
    nss_SetError(NSS_ERROR_INTERNAL_ERROR); 
    break;
  default:
    nss_SetError(NSS_ERROR_UNSUPPORTED_TYPE);
    break;
  }

  return rv;
}












NSS_IMPLEMENT PRStatus
nssUTF8_CopyIntoFixedBuffer
(
  NSSUTF8 *string,
  char *buffer,
  PRUint32 bufferSize,
  char pad
)
{
  PRUint32 stringSize = 0;

#ifdef NSSDEBUG
  if( (char *)NULL == buffer ) {
    nss_SetError(NSS_ERROR_INVALID_POINTER);
    return PR_FALSE;
  }

  if( 0 == bufferSize ) {
    nss_SetError(NSS_ERROR_INVALID_ARGUMENT);
    return PR_FALSE;
  }

  if( (pad & 0x80) != 0x00 ) {
    nss_SetError(NSS_ERROR_INVALID_ARGUMENT);
    return PR_FALSE;
  }
#endif 

  if( (NSSUTF8 *)NULL == string ) {
    string = (NSSUTF8 *) "";
  }

  stringSize = nssUTF8_Size(string, (PRStatus *)NULL);
  stringSize--; 
  if( stringSize > bufferSize ) {
    PRUint32 bs = bufferSize;
    (void)nsslibc_memcpy(buffer, string, bufferSize);
    
    if( (            ((buffer[ bs-1 ] & 0x80) == 0x00)) ||
        ((bs > 1) && ((buffer[ bs-2 ] & 0xE0) == 0xC0)) ||
        ((bs > 2) && ((buffer[ bs-3 ] & 0xF0) == 0xE0)) ||
        ((bs > 3) && ((buffer[ bs-4 ] & 0xF8) == 0xF0)) ||
        ((bs > 4) && ((buffer[ bs-5 ] & 0xFC) == 0xF8)) ||
        ((bs > 5) && ((buffer[ bs-6 ] & 0xFE) == 0xFC)) ) {
      
      return PR_SUCCESS;
    }

    
    for( ; bs != 0; bs-- ) {
      if( (buffer[bs-1] & 0xC0) != 0x80 ) {
        buffer[bs-1] = pad;
        break;
      } else {
        buffer[bs-1] = pad;
      }
    }      
  } else {
    (void)nsslibc_memset(buffer, pad, bufferSize);
    (void)nsslibc_memcpy(buffer, string, stringSize);
  }

  return PR_SUCCESS;
}






NSS_IMPLEMENT PRBool
nssUTF8_Equal
(
  const NSSUTF8 *a,
  const NSSUTF8 *b,
  PRStatus *statusOpt
)
{
  PRUint32 la, lb;

#ifdef NSSDEBUG
  if( ((const NSSUTF8 *)NULL == a) ||
      ((const NSSUTF8 *)NULL == b) ) {
    nss_SetError(NSS_ERROR_INVALID_POINTER);
    if( (PRStatus *)NULL != statusOpt ) {
      *statusOpt = PR_FAILURE;
    }
    return PR_FALSE;
  }
#endif 

  la = nssUTF8_Size(a, statusOpt);
  if( 0 == la ) {
    return PR_FALSE;
  }

  lb = nssUTF8_Size(b, statusOpt);
  if( 0 == lb ) {
    return PR_FALSE;
  }

  if( la != lb ) {
    return PR_FALSE;
  }

  return nsslibc_memequal(a, b, la, statusOpt);
}
