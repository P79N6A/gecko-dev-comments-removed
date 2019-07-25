



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


NS_IMETHODIMP mozGenericWordUtils::GetRootForm(const PRUnichar *word, uint32_t type, PRUnichar ***words, uint32_t *count)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP mozGenericWordUtils::FromRootForm(const PRUnichar *word, const PRUnichar **iwords, uint32_t icount, PRUnichar ***owords, uint32_t *ocount)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP mozGenericWordUtils::FindNextWord(const PRUnichar *word, uint32_t length, uint32_t offset, int32_t *begin, int32_t *end)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

