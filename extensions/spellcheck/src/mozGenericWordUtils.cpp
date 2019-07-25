



































#include "mozGenericWordUtils.h"

NS_IMPL_CYCLE_COLLECTING_ADDREF(mozGenericWordUtils)
NS_IMPL_CYCLE_COLLECTING_RELEASE(mozGenericWordUtils)

NS_INTERFACE_MAP_BEGIN(mozGenericWordUtils)
  NS_INTERFACE_MAP_ENTRY(mozISpellI18NUtil)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, mozISpellI18NUtil)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(mozGenericWordUtils)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_0(mozGenericWordUtils)

  

mozGenericWordUtils::mozGenericWordUtils()
{
  
}

mozGenericWordUtils::~mozGenericWordUtils()
{
  
}


NS_IMETHODIMP mozGenericWordUtils::GetLanguage(PRUnichar * *aLanguage)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP mozGenericWordUtils::GetRootForm(const PRUnichar *word, PRUint32 type, PRUnichar ***words, PRUint32 *count)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP mozGenericWordUtils::FromRootForm(const PRUnichar *word, const PRUnichar **iwords, PRUint32 icount, PRUnichar ***owords, PRUint32 *ocount)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP mozGenericWordUtils::FindNextWord(const PRUnichar *word, PRUint32 length, PRUint32 offset, PRInt32 *begin, PRInt32 *end)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

