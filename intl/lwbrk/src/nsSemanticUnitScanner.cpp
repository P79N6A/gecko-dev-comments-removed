




#include "nsSemanticUnitScanner.h"
#include "prmem.h"

NS_IMPL_ISUPPORTS1(nsSemanticUnitScanner, nsISemanticUnitScanner)

nsSemanticUnitScanner::nsSemanticUnitScanner() : nsSampleWordBreaker()
{
  
}

nsSemanticUnitScanner::~nsSemanticUnitScanner()
{
  
}



NS_IMETHODIMP nsSemanticUnitScanner::Start(const char *characterSet)
{
    
    return NS_OK;
}


NS_IMETHODIMP nsSemanticUnitScanner::Next(const PRUnichar *text, int32_t length, int32_t pos, bool isLastBuffer, int32_t *begin, int32_t *end, bool *_retval)
{
    
    

    
    if (pos >= length) {
       *begin = pos;
       *end = pos;
       *_retval = false;
       return NS_OK;
    }

    uint8_t char_class = nsSampleWordBreaker::GetClass(text[pos]);

    
    
    if (kWbClassHanLetter == char_class) {
       *begin = pos;
       *end = pos+1;
       *_retval = true;
       return NS_OK;
    }

    int32_t next;
    
    next = NextWord(text, (uint32_t) length, (uint32_t) pos);

    
    if (next == NS_WORDBREAKER_NEED_MORE_TEXT) {
       *begin = pos;
       *end = isLastBuffer ? length : pos;
       *_retval = isLastBuffer;
       return NS_OK;
    } 
    
    
    if ((char_class == kWbClassSpace) || (char_class == kWbClassPunct)) {
        
        
        return Next(text, length, next, isLastBuffer, begin, end, _retval);
    }

    
    *begin = pos;
    *end = next;
    *_retval = true;
    return NS_OK;
}

