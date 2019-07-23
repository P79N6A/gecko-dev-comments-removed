




































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


NS_IMETHODIMP nsSemanticUnitScanner::Next(const PRUnichar *text, PRInt32 length, PRInt32 pos, PRBool isLastBuffer, PRInt32 *begin, PRInt32 *end, PRBool *_retval)
{
    
    

    
    if (pos >= length) {
       *begin = pos;
       *end = pos;
       *_retval = PR_FALSE;
       return NS_OK;
    }

    PRUint8 char_class = nsSampleWordBreaker::GetClass(text[pos]);

    
    
    if (kWbClassHanLetter == char_class) {
       *begin = pos;
       *end = pos+1;
       *_retval = PR_TRUE;
       return NS_OK;
    }

    PRInt32 next;
    
    next = NextWord(text, (PRUint32) length, (PRUint32) pos);

    
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
    *_retval = PR_TRUE;
    return NS_OK;
}

