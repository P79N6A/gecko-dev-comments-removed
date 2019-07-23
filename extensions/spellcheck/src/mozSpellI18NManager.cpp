




































#include "mozSpellI18NManager.h"
#include "mozEnglishWordUtils.h"
#include "mozGenericWordUtils.h"
#include "nsString.h"

NS_IMPL_ISUPPORTS1(mozSpellI18NManager, mozISpellI18NManager)

mozSpellI18NManager::mozSpellI18NManager()
{
  
}

mozSpellI18NManager::~mozSpellI18NManager()
{
  
}


NS_IMETHODIMP mozSpellI18NManager::GetUtil(const PRUnichar *aLanguage, mozISpellI18NUtil **_retval)
{
 if( NULL == _retval) {
   return NS_ERROR_NULL_POINTER;
 }
 *_retval = NULL;
 nsAutoString lang;
 lang.Assign(aLanguage);
 if(lang.EqualsLiteral("en")){
   *_retval = new mozEnglishWordUtils;
 }
 else{
   *_retval = new mozEnglishWordUtils;   
 }

 NS_IF_ADDREF(*_retval);
 return NS_OK;
}
