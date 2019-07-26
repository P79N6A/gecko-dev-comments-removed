



#include "mozGenericWordUtils.h"

NS_IMPL_ISUPPORTS1(mozGenericWordUtils, mozISpellI18NUtil)

  

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

