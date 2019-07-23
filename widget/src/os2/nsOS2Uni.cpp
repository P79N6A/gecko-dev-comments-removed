



































#include "nsOS2Uni.h"

int WideCharToMultiByte( int CodePage, const PRUnichar *pText, ULONG ulLength, char* szBuffer, ULONG ulSize )
{
  UconvObject Converter = OS2Uni::GetUconvObject(CodePage);

  UniChar *ucsString = (UniChar*) pText;
  size_t   ucsLen = ulLength;
  size_t   cplen = ulSize;
  size_t   cSubs = 0;

  char *tmp = szBuffer; 

  int unirc = ::UniUconvFromUcs( Converter, &ucsString, &ucsLen,
                                 (void**) &tmp, &cplen, &cSubs);

  if( unirc != ULS_SUCCESS )
    return 0;

  if( unirc == UCONV_E2BIG)
  {
    
    *(szBuffer + ulSize - 1) = '\0';
  }

  return ulSize - cplen;
}

int MultiByteToWideChar( int CodePage, const char*pText, ULONG ulLength, PRUnichar *szBuffer, ULONG ulSize )
{
  UconvObject Converter = OS2Uni::GetUconvObject(CodePage);

  char *ucsString = (char*) pText;
  size_t   ucsLen = ulLength;
  size_t   cplen = ulSize;
  size_t   cSubs = 0;

  PRUnichar *tmp = szBuffer; 

  int unirc = ::UniUconvToUcs( Converter, (void**)&ucsString, &ucsLen,
                               NS_REINTERPRET_CAST(UniChar**, &tmp),
                               &cplen, &cSubs);
                               
  if( unirc != ULS_SUCCESS )
    return 0;

  if( unirc == UCONV_E2BIG)
  {
    
    *(szBuffer + ulSize - 1) = '\0';
  }

  return ulSize - cplen;
}

nsHashtable OS2Uni::gUconvObjects;

UconvObject
OS2Uni::GetUconvObject(int CodePage)
{
  nsPRUint32Key key(CodePage);
  UconvObject uco = OS2Uni::gUconvObjects.Get(&key);
  if (!uco) {
    UniChar codepage[20];
    int unirc = ::UniMapCpToUcsCp(CodePage, codepage, 20);
    if (unirc == ULS_SUCCESS) {
       unirc = ::UniCreateUconvObject(codepage, &uco);
       if (unirc == ULS_SUCCESS) {
          uconv_attribute_t attr;

          ::UniQueryUconvObject(uco, &attr, sizeof(uconv_attribute_t), 
                                NULL, NULL, NULL);
          attr.options = UCONV_OPTION_SUBSTITUTE_BOTH;
          attr.subchar_len=1;
          attr.subchar[0]='?';
          ::UniSetUconvObject(uco, &attr);
          OS2Uni::gUconvObjects.Put(&key, uco);
       }
    }
  }
  return uco;
}

PR_STATIC_CALLBACK(PRIntn)
UconvObjectEnum(nsHashKey* hashKey, void *aData, void* closure)
{
  UniFreeUconvObject((UconvObject)aData);
  return kHashEnumerateRemove;
}

void OS2Uni::FreeUconvObjects()
{
  if (gUconvObjects.Count()) {
    gUconvObjects.Enumerate(UconvObjectEnum, nsnull);
  }
}
