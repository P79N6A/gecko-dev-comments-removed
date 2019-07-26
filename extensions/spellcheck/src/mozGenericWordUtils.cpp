



#include "mozGenericWordUtils.h"

NS_IMPL_ISUPPORTS(mozGenericWordUtils, mozISpellI18NUtil)

  

mozGenericWordUtils::mozGenericWordUtils()
{
  
}

mozGenericWordUtils::~mozGenericWordUtils()
{
  
}


NS_IMETHODIMP mozGenericWordUtils::GetLanguage(char16_t * *aLanguage)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP mozGenericWordUtils::GetRootForm(const char16_t *word, uint32_t type, char16_t ***words, uint32_t *count)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP mozGenericWordUtils::FromRootForm(const char16_t *word, const char16_t **iwords, uint32_t icount, char16_t ***owords, uint32_t *ocount)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP mozGenericWordUtils::FindNextWord(const char16_t *word, uint32_t length, uint32_t offset, int32_t *begin, int32_t *end)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

