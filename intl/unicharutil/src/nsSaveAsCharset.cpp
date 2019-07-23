






































#include "prmem.h"
#include "prprf.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsICharsetConverterManager.h"
#include "nsSaveAsCharset.h"
#include "nsCRT.h"
#include "nsUnicharUtils.h"
#include "nsCompressedCharMap.h"




NS_IMPL_ISUPPORTS1(nsSaveAsCharset, nsISaveAsCharset)

#include "ignorables_abjadpoints.x-ccmap"
DEFINE_X_CCMAP(gIgnorableCCMapExt, const);




nsSaveAsCharset::nsSaveAsCharset()
{
  mAttribute = attr_htmlTextDefault;
  mEntityVersion = 0;
  mCharsetListIndex = -1;
}

nsSaveAsCharset::~nsSaveAsCharset()
{
}

NS_IMETHODIMP
nsSaveAsCharset::Init(const char *charset, PRUint32 attr, PRUint32 entityVersion)
{
  nsresult rv = NS_OK;

  mAttribute = attr;
  mEntityVersion = entityVersion;

  rv = SetupCharsetList(charset);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = SetupUnicodeEncoder(GetNextCharset());
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (attr_EntityNone != MASK_ENTITY(mAttribute) && !mEntityConverter)
    mEntityConverter = do_CreateInstance(NS_ENTITYCONVERTER_CONTRACTID, &rv);

  return rv;
}

NS_IMETHODIMP
nsSaveAsCharset::Convert(const PRUnichar *inString, char **_retval)
{
  if (nsnull == _retval)
    return NS_ERROR_NULL_POINTER;
  if (nsnull == inString)
    return NS_ERROR_NULL_POINTER;
  if (0 == *inString)
    return NS_ERROR_ILLEGAL_VALUE;
  nsresult rv = NS_OK;

  NS_ASSERTION(mEncoder, "need to call Init() before Convert()");
  NS_ENSURE_TRUE(mEncoder, NS_ERROR_FAILURE);

  *_retval = nsnull;

  
  if (mCharsetListIndex > 0) {
    mCharsetListIndex = -1;
    rv = SetupUnicodeEncoder(GetNextCharset());
    NS_ENSURE_SUCCESS(rv, rv);
  }

  do {
    
    if (MASK_CHARSET_FALLBACK(mAttribute) && NS_ERROR_UENC_NOMAPPING == rv) {
      const char * charset = GetNextCharset();
      if (!charset)
        break;
      rv = SetupUnicodeEncoder(charset);
      NS_ENSURE_SUCCESS(rv, rv);
      PR_FREEIF(*_retval);
    }

    if (attr_EntityBeforeCharsetConv == MASK_ENTITY(mAttribute)) {
      NS_ASSERTION(mEntityConverter, "need to call Init() before Convert()");
      NS_ENSURE_TRUE(mEntityConverter, NS_ERROR_FAILURE);
      PRUnichar *entity = nsnull;
      
      rv = mEntityConverter->ConvertToEntities(inString, mEntityVersion, &entity);
      if(NS_SUCCEEDED(rv)) {
        rv = DoCharsetConversion(entity, _retval);
        nsMemory::Free(entity);
      }
    }
    else
      rv = DoCharsetConversion(inString, _retval);

  } while (MASK_CHARSET_FALLBACK(mAttribute) && NS_ERROR_UENC_NOMAPPING == rv);

  return rv;
}

NS_IMETHODIMP 
nsSaveAsCharset::GetCharset(char * *aCharset)
{
  NS_ENSURE_ARG(aCharset);
  NS_ASSERTION(mCharsetListIndex >= 0, "need to call Init() first");
  NS_ENSURE_TRUE(mCharsetListIndex >= 0, NS_ERROR_FAILURE);

  const char *charset = mCharsetList[mCharsetListIndex]->get();
  if (!charset) {
    *aCharset = nsnull;
    NS_ASSERTION(charset, "make sure to call Init() with non empty charset list");
    return NS_ERROR_FAILURE;
  }

  *aCharset = nsCRT::strdup(charset);
  return (*aCharset) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}





NS_IMETHODIMP
nsSaveAsCharset::HandleFallBack(PRUint32 character, char **outString, PRInt32 *bufferLength, 
                                PRInt32 *currentPos, PRInt32 estimatedLength)
{
  if((nsnull == outString ) || (nsnull == bufferLength) ||(nsnull ==currentPos))
    return NS_ERROR_NULL_POINTER;
  char fallbackStr[256];
  nsresult rv = DoConversionFallBack(character, fallbackStr, 256);
  if (NS_SUCCEEDED(rv)) {
    PRInt32 tempLen = (PRInt32) PL_strlen(fallbackStr);

    
    if ((tempLen + estimatedLength) >= (*bufferLength - *currentPos)) {
      char *temp = (char *) PR_Realloc(*outString, *bufferLength + tempLen);
      if (NULL != temp) {
        
        *bufferLength += tempLen;
        *outString = temp;
      } else {
        *outString = NULL;
        *bufferLength =0;
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }
    memcpy((*outString + *currentPos), fallbackStr, tempLen);
    *currentPos += tempLen;
  }
  return rv;
}

NS_IMETHODIMP
nsSaveAsCharset::DoCharsetConversion(const PRUnichar *inString, char **outString)
{
  if(nsnull == outString )
    return NS_ERROR_NULL_POINTER;
  NS_ASSERTION(outString, "invalid input");

  *outString = NULL;

  nsresult rv;
  PRInt32 inStringLength = nsCRT::strlen(inString);   
  PRInt32 bufferLength;                               
  PRInt32 srcLength = inStringLength;
  PRInt32 dstLength;
  char *dstPtr = NULL;
  PRInt32 pos1, pos2;
  nsresult saveResult = NS_OK;                         

  
  rv = mEncoder->GetMaxLength(inString, inStringLength, &dstLength);
  if (NS_FAILED(rv)) return rv;

  bufferLength = dstLength + 512; 
  dstPtr = (char *) PR_Malloc(bufferLength);
  if (NULL == dstPtr) return NS_ERROR_OUT_OF_MEMORY;

  
  for (pos1 = 0, pos2 = 0; pos1 < inStringLength;) {
    
    dstLength = bufferLength - pos2;
    rv = mEncoder->Convert(&inString[pos1], &srcLength, &dstPtr[pos2], &dstLength);

    pos1 += srcLength ? srcLength : 1;
    pos2 += dstLength;
    dstPtr[pos2] = '\0';

    
    if (NS_ERROR_UENC_NOMAPPING != rv) break;

    
    saveResult = rv;
    rv = NS_OK;

    
    dstLength = bufferLength - pos2;
    rv = mEncoder->Finish(&dstPtr[pos2], &dstLength);
    if (NS_SUCCEEDED(rv)) {
      pos2 += dstLength;
      dstPtr[pos2] = '\0';
    }

    srcLength = inStringLength - pos1;

    
    if (!ATTR_NO_FALLBACK(mAttribute)) {
      PRUint32 unMappedChar;
      if (NS_IS_HIGH_SURROGATE(inString[pos1-1]) && 
          inStringLength > pos1 && NS_IS_LOW_SURROGATE(inString[pos1])) {
        unMappedChar = SURROGATE_TO_UCS4(inString[pos1-1], inString[pos1]);
        pos1++;
      } else {
        unMappedChar = inString[pos1-1];
      }

      
      if (MASK_IGNORABLE_FALLBACK(mAttribute) &&
          CCMAP_HAS_CHAR_EXT(gIgnorableCCMapExt, unMappedChar)) 
				continue;

      rv = mEncoder->GetMaxLength(inString+pos1, inStringLength-pos1, &dstLength);
      if (NS_FAILED(rv)) 
        break;

      rv = HandleFallBack(unMappedChar, &dstPtr, &bufferLength, &pos2, dstLength);
      if (NS_FAILED(rv)) 
        break;
      dstPtr[pos2] = '\0';
    }
  }

  if (NS_SUCCEEDED(rv)) {
    
    dstLength = bufferLength - pos2;
    rv = mEncoder->Finish(&dstPtr[pos2], &dstLength);
    if (NS_SUCCEEDED(rv)) {
      pos2 += dstLength;
      dstPtr[pos2] = '\0';
    }
  }

  if (NS_FAILED(rv)) {
    PR_FREEIF(dstPtr);
    return rv;
  }

  *outString = dstPtr;      

  
  if (NS_ERROR_UENC_NOMAPPING == saveResult) {
    rv = NS_ERROR_UENC_NOMAPPING;
  }

  return rv;
}

NS_IMETHODIMP
nsSaveAsCharset::DoConversionFallBack(PRUint32 inUCS4, char *outString, PRInt32 bufferLength)
{
  NS_ASSERTION(outString, "invalid input");
  if(nsnull == outString )
    return NS_ERROR_NULL_POINTER;

  *outString = '\0';

  nsresult rv = NS_OK;

  if (ATTR_NO_FALLBACK(mAttribute)) {
    return NS_OK;
  }
  if (attr_EntityAfterCharsetConv == MASK_ENTITY(mAttribute)) {
    char *entity = NULL;
    rv = mEntityConverter->ConvertUTF32ToEntity(inUCS4, mEntityVersion, &entity);
    if (NS_SUCCEEDED(rv)) {
      if (NULL == entity || (PRInt32)strlen(entity) > bufferLength) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      PL_strcpy(outString, entity);
      nsMemory::Free(entity);
      return rv;
    }
  }

  switch (MASK_FALLBACK(mAttribute)) {
  case attr_FallbackQuestionMark:
    if(bufferLength>=2) {
      *outString++='?';
      *outString='\0';
      rv = NS_OK;
    } else {
      rv = NS_ERROR_FAILURE;
    }
    break;
  case attr_FallbackEscapeU:
    if (inUCS4 & 0xff0000)
      rv = (PR_snprintf(outString, bufferLength, "\\u%.6x", inUCS4) > 0) ? NS_OK : NS_ERROR_FAILURE;
    else
      rv = (PR_snprintf(outString, bufferLength, "\\u%.4x", inUCS4) > 0) ? NS_OK : NS_ERROR_FAILURE;
    break;
  case attr_FallbackDecimalNCR:
    rv = ( PR_snprintf(outString, bufferLength, "&#%u;", inUCS4) > 0) ? NS_OK : NS_ERROR_FAILURE;
    break;
  case attr_FallbackHexNCR:
    rv = (PR_snprintf(outString, bufferLength, "&#x%x;", inUCS4) > 0) ? NS_OK : NS_ERROR_FAILURE;
    break;
  case attr_FallbackNone:
    rv = NS_OK;
    break;
  default:
    rv = NS_ERROR_ILLEGAL_VALUE;
    break;
  }

	return rv;
}

nsresult nsSaveAsCharset::SetupUnicodeEncoder(const char* charset)
{
  NS_ENSURE_ARG(charset);
  nsresult rv;

  
  nsCOMPtr <nsICharsetConverterManager> ccm = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return ccm->GetUnicodeEncoder(charset, getter_AddRefs(mEncoder));
}

nsresult nsSaveAsCharset::SetupCharsetList(const char *charsetList)
{
  NS_ENSURE_ARG(charsetList);

  NS_ASSERTION(charsetList[0], "charsetList should not be empty");
  if (!charsetList[0])
    return NS_ERROR_INVALID_ARG;

  if (mCharsetListIndex >= 0) {
    mCharsetList.Clear();
    mCharsetListIndex = -1;
  }

  mCharsetList.ParseString(charsetList, ", ");

  return NS_OK;
}

const char * nsSaveAsCharset::GetNextCharset()
{
  if ((mCharsetListIndex + 1) >= mCharsetList.Count())
    return nsnull;

  
  return mCharsetList[++mCharsetListIndex]->get();
}



nsresult 
NS_NewSaveAsCharset(nsISupports **inst)
{
  if(nsnull == inst )
    return NS_ERROR_NULL_POINTER;
  *inst = (nsISupports *) new nsSaveAsCharset;
   if(*inst)
      NS_ADDREF(*inst);
   return (*inst) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}
