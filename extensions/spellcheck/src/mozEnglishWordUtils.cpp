




#include "mozEnglishWordUtils.h"
#include "nsReadableUtils.h"
#include "nsIServiceManager.h"
#include "nsUnicharUtils.h"
#include "nsUnicharUtilCIID.h"
#include "nsUnicodeProperties.h"
#include "nsCRT.h"
#include "mozilla/Likely.h"

NS_IMPL_CYCLE_COLLECTING_ADDREF(mozEnglishWordUtils)
NS_IMPL_CYCLE_COLLECTING_RELEASE(mozEnglishWordUtils)

NS_INTERFACE_MAP_BEGIN(mozEnglishWordUtils)
  NS_INTERFACE_MAP_ENTRY(mozISpellI18NUtil)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, mozISpellI18NUtil)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(mozEnglishWordUtils)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION(mozEnglishWordUtils,
                         mURLDetector)

mozEnglishWordUtils::mozEnglishWordUtils()
{
  mLanguage.AssignLiteral("en");

  nsresult rv;
  mURLDetector = do_CreateInstance(MOZ_TXTTOHTMLCONV_CONTRACTID, &rv);
}

mozEnglishWordUtils::~mozEnglishWordUtils()
{
}


NS_IMETHODIMP mozEnglishWordUtils::GetLanguage(char16_t * *aLanguage)
{
  nsresult rv = NS_OK;
  NS_ENSURE_ARG_POINTER(aLanguage);

  *aLanguage = ToNewUnicode(mLanguage);
  if(!aLanguage) rv = NS_ERROR_OUT_OF_MEMORY;
  return rv;
 }



NS_IMETHODIMP mozEnglishWordUtils::GetRootForm(const char16_t *aWord, uint32_t type, char16_t ***words, uint32_t *count)
{
  nsAutoString word(aWord);
  char16_t **tmpPtr;
  int32_t length = word.Length();

  *count = 0;

  mozEnglishWordUtils::myspCapitalization ct = captype(word);
  switch (ct)
    {
    case HuhCap:
    case NoCap: 
      tmpPtr = (char16_t **)moz_xmalloc(sizeof(char16_t *));
      if (!tmpPtr)
        return NS_ERROR_OUT_OF_MEMORY;
      tmpPtr[0] = ToNewUnicode(word);
      if (!tmpPtr[0]) {
        NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(0, tmpPtr);
        return NS_ERROR_OUT_OF_MEMORY;
      }
      *words = tmpPtr;
      *count = 1;
      break;
    

    case AllCap:
      tmpPtr = (char16_t **)moz_xmalloc(sizeof(char16_t *) * 3);
      if (!tmpPtr)
        return NS_ERROR_OUT_OF_MEMORY;
      tmpPtr[0] = ToNewUnicode(word);
      if (!tmpPtr[0]) {
        NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(0, tmpPtr);
        return NS_ERROR_OUT_OF_MEMORY;
      }
      ToLowerCase(tmpPtr[0], tmpPtr[0], length);

      tmpPtr[1] = ToNewUnicode(word);
      if (!tmpPtr[1]) {
        NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(1, tmpPtr);
        return NS_ERROR_OUT_OF_MEMORY;
      }
      ToLowerCase(tmpPtr[1], tmpPtr[1], length);
      ToUpperCase(tmpPtr[1], tmpPtr[1], 1);

      tmpPtr[2] = ToNewUnicode(word);
      if (!tmpPtr[2]) {
        NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(2, tmpPtr);
        return NS_ERROR_OUT_OF_MEMORY;
      }

      *words = tmpPtr;
      *count = 3;
      break;
 
    case InitCap:  
      tmpPtr = (char16_t **)moz_xmalloc(sizeof(char16_t *) * 2);
      if (!tmpPtr)
        return NS_ERROR_OUT_OF_MEMORY;

      tmpPtr[0] = ToNewUnicode(word);
      if (!tmpPtr[0]) {
        NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(0, tmpPtr);
        return NS_ERROR_OUT_OF_MEMORY;
      }
      ToLowerCase(tmpPtr[0], tmpPtr[0], length);

      tmpPtr[1] = ToNewUnicode(word);
      if (!tmpPtr[1]) {
        NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(1, tmpPtr);
        return NS_ERROR_OUT_OF_MEMORY;
      }

      *words = tmpPtr;
      *count = 2;
      break;
    default:
      return NS_ERROR_FAILURE; 
    }
  return NS_OK;
}


bool mozEnglishWordUtils::ucIsAlpha(char16_t aChar)
{
  
  return nsIUGenCategory::kLetter == mozilla::unicode::GetGenCategory(aChar);
}


NS_IMETHODIMP mozEnglishWordUtils::FindNextWord(const char16_t *word, uint32_t length, uint32_t offset, int32_t *begin, int32_t *end)
{
  const char16_t *p = word + offset;
  const char16_t *endbuf = word + length;
  const char16_t *startWord=p;
  if(p<endbuf){
    
    
    if (offset > 0 && ucIsAlpha(*(p-1))) {
      while (p < endbuf && ucIsAlpha(*p))
        p++;
    }
    while((p < endbuf) && (!ucIsAlpha(*p)))
      {
        p++;
      }
    startWord=p;
    while((p < endbuf) && ((ucIsAlpha(*p))||(*p=='\'')))
      { 
        p++;
      }
    
    
    
    

    
    
    if ( (*p == ':' || *p == '@' || *p == '.') &&  p < endbuf - 1) {

        
        
       
        if (mURLDetector)
        {
          int32_t startPos = -1;
          int32_t endPos = -1;        

          mURLDetector->FindURLInPlaintext(startWord, endbuf - startWord, p - startWord, &startPos, &endPos);

          
          if (startPos != -1 && endPos != -1) { 
            startWord = p + endPos + 1; 
            p = startWord; 

            
            return FindNextWord(word, length, startWord - word, begin, end);
          }
        }
    }

    while((p > startWord)&&(*(p-1) == '\'')){  
      p--;
    }
  }
  else{
    startWord = endbuf;
  }
  if(startWord == endbuf){
    *begin = -1;
    *end = -1;
  }
  else{
    *begin = startWord-word;
    *end = p-word;
  }
  return NS_OK;
}

mozEnglishWordUtils::myspCapitalization 
mozEnglishWordUtils::captype(const nsString &word)
{
  char16_t* lword=ToNewUnicode(word);  
  ToUpperCase(lword,lword,word.Length());
  if(word.Equals(lword)){
    free(lword);
    return AllCap;
  }

  ToLowerCase(lword,lword,word.Length());
  if(word.Equals(lword)){
    free(lword);
    return NoCap;
  }
  int32_t length=word.Length();
  if(Substring(word,1,length-1).Equals(lword+1)){
    free(lword);
    return InitCap;
  }
  free(lword);
  return HuhCap;
}



NS_IMETHODIMP mozEnglishWordUtils::FromRootForm(const char16_t *aWord, const char16_t **iwords, uint32_t icount, char16_t ***owords, uint32_t *ocount)
{
  nsAutoString word(aWord);
  nsresult rv = NS_OK;

  int32_t length;
  char16_t **tmpPtr  = (char16_t **)moz_xmalloc(sizeof(char16_t *)*icount);
  if (!tmpPtr)
    return NS_ERROR_OUT_OF_MEMORY;

  mozEnglishWordUtils::myspCapitalization ct = captype(word);
  for(uint32_t i = 0; i < icount; ++i) {
    length = NS_strlen(iwords[i]);
    tmpPtr[i] = (char16_t *) moz_xmalloc(sizeof(char16_t) * (length + 1));
    if (MOZ_UNLIKELY(!tmpPtr[i])) {
      NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(i, tmpPtr);
      return NS_ERROR_OUT_OF_MEMORY;
    }
    memcpy(tmpPtr[i], iwords[i], (length + 1) * sizeof(char16_t));

    nsAutoString capTest(tmpPtr[i]);
    mozEnglishWordUtils::myspCapitalization newCt=captype(capTest);
    if(newCt == NoCap){
      switch(ct) 
        {
        case HuhCap:
        case NoCap:
          break;
        case AllCap:
          ToUpperCase(tmpPtr[i],tmpPtr[i],length);
          rv = NS_OK;
          break;
        case InitCap:  
          ToUpperCase(tmpPtr[i],tmpPtr[i],1);
          rv = NS_OK;
          break;
        default:
          rv = NS_ERROR_FAILURE; 
          break;

        }
    }
  }
  if (NS_SUCCEEDED(rv)){
    *owords = tmpPtr;
    *ocount = icount;
  }
  return rv;
}

