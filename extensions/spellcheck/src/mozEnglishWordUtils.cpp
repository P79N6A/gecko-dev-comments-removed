




































#include "mozEnglishWordUtils.h"
#include "nsICharsetAlias.h"
#include "nsReadableUtils.h"
#include "nsIServiceManager.h"
#include "nsUnicharUtilCIID.h"
#include "nsCRT.h"
#include "cattable.h"

NS_IMPL_ISUPPORTS1(mozEnglishWordUtils, mozISpellI18NUtil)

mozEnglishWordUtils::mozEnglishWordUtils()
{
  mLanguage.AssignLiteral("en");

  nsresult rv;
  mURLDetector = do_CreateInstance(MOZ_TXTTOHTMLCONV_CONTRACTID, &rv);
}

mozEnglishWordUtils::~mozEnglishWordUtils()
{
}


NS_IMETHODIMP mozEnglishWordUtils::GetLanguage(PRUnichar * *aLanguage)
{
  nsresult rv = NS_OK;
  NS_ENSURE_ARG_POINTER(aLanguage);

  *aLanguage = ToNewUnicode(mLanguage);
  if(!aLanguage) rv = NS_ERROR_OUT_OF_MEMORY;
  return rv;
 }



NS_IMETHODIMP mozEnglishWordUtils::GetRootForm(const PRUnichar *aWord, PRUint32 type, PRUnichar ***words, PRUint32 *count)
{
  nsAutoString word(aWord);
  PRUnichar **tmpPtr;
  PRInt32 length = word.Length();

  *count = 0;

  if (!mCaseConv) {
    mCaseConv = do_GetService(NS_UNICHARUTIL_CONTRACTID);
    if (!mCaseConv)
      return NS_ERROR_FAILURE;
  }

  mozEnglishWordUtils::myspCapitalization ct = captype(word);
  switch (ct)
    {
    case HuhCap:
    case NoCap: 
      tmpPtr = (PRUnichar **)nsMemory::Alloc(sizeof(PRUnichar *));
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
      tmpPtr = (PRUnichar **)nsMemory::Alloc(sizeof(PRUnichar *) * 3);
      if (!tmpPtr)
        return NS_ERROR_OUT_OF_MEMORY;
      tmpPtr[0] = ToNewUnicode(word);
      if (!tmpPtr[0]) {
        NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(0, tmpPtr);
        return NS_ERROR_OUT_OF_MEMORY;
      }
      mCaseConv->ToLower(tmpPtr[0], tmpPtr[0], length);

      tmpPtr[1] = ToNewUnicode(word);
      if (!tmpPtr[1]) {
        NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(1, tmpPtr);
        return NS_ERROR_OUT_OF_MEMORY;
      }
      mCaseConv->ToLower(tmpPtr[1], tmpPtr[1], length);
      mCaseConv->ToUpper(tmpPtr[1], tmpPtr[1], 1);

      tmpPtr[2] = ToNewUnicode(word);
      if (!tmpPtr[2]) {
        NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(2, tmpPtr);
        return NS_ERROR_OUT_OF_MEMORY;
      }

      *words = tmpPtr;
      *count = 3;
      break;
 
    case InitCap:  
      tmpPtr = (PRUnichar **)nsMemory::Alloc(sizeof(PRUnichar *) * 2);
      if (!tmpPtr)
        return NS_ERROR_OUT_OF_MEMORY;

      tmpPtr[0] = ToNewUnicode(word);
      if (!tmpPtr[0]) {
        NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(0, tmpPtr);
        return NS_ERROR_OUT_OF_MEMORY;
      }
      mCaseConv->ToLower(tmpPtr[0], tmpPtr[0], length);

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


static PRBool ucIsAlpha(PRUnichar c)
{
  
  return (5 == GetCat(PRUint32(c)));
}


NS_IMETHODIMP mozEnglishWordUtils::FindNextWord(const PRUnichar *word, PRUint32 length, PRUint32 offset, PRInt32 *begin, PRInt32 *end)
{
  const PRUnichar *p = word + offset;
  const PRUnichar *endbuf = word + length;
  const PRUnichar *startWord=p;
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
          PRInt32 startPos = -1;
          PRInt32 endPos = -1;        

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
  if(!mCaseConv) return HuhCap; 
  PRUnichar* lword=ToNewUnicode(word);  
  mCaseConv->ToUpper(lword,lword,word.Length());
  if(word.Equals(lword)){
    nsMemory::Free(lword);
    return AllCap;
  }

  mCaseConv->ToLower(lword,lword,word.Length());
  if(word.Equals(lword)){
    nsMemory::Free(lword);
    return NoCap;
  }
  PRInt32 length=word.Length();
  if(Substring(word,1,length-1).Equals(lword+1)){
    nsMemory::Free(lword);
    return InitCap;
  }
  nsMemory::Free(lword);
  return HuhCap;
}



NS_IMETHODIMP mozEnglishWordUtils::FromRootForm(const PRUnichar *aWord, const PRUnichar **iwords, PRUint32 icount, PRUnichar ***owords, PRUint32 *ocount)
{
  nsAutoString word(aWord);
  nsresult rv = NS_OK;

  PRInt32 length;
  PRUnichar **tmpPtr  = (PRUnichar **)nsMemory::Alloc(sizeof(PRUnichar *)*icount);
  if (!tmpPtr)
    return NS_ERROR_OUT_OF_MEMORY;

  mozEnglishWordUtils::myspCapitalization ct = captype(word);
  for(PRUint32 i = 0; i < icount; ++i) {
    length = nsCRT::strlen(iwords[i]);
    tmpPtr[i] = (PRUnichar *) nsMemory::Alloc(sizeof(PRUnichar) * (length + 1));
    if (NS_UNLIKELY(!tmpPtr[i])) {
      NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(i, tmpPtr);
      return NS_ERROR_OUT_OF_MEMORY;
    }
    memcpy(tmpPtr[i], iwords[i], (length + 1) * sizeof(PRUnichar));

    nsAutoString capTest(tmpPtr[i]);
    mozEnglishWordUtils::myspCapitalization newCt=captype(capTest);
    if(newCt == NoCap){
      switch(ct) 
        {
        case HuhCap:
        case NoCap:
          break;
        case AllCap:
          rv = mCaseConv->ToUpper(tmpPtr[i],tmpPtr[i],length);
          break;
        case InitCap:  
          rv = mCaseConv->ToUpper(tmpPtr[i],tmpPtr[i],1);
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

