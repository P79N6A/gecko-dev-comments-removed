




































#include "nscore.h"
#include "pratom.h"
#include "nsUUDll.h"
#include "nsISupports.h"
#include "nsCategoryImp.h"
#include "cattable.h"

NS_IMPL_ISUPPORTS1(nsCategoryImp, nsIUGenCategory)


nsCategoryImp::nsCategoryImp()
{
}

nsCategoryImp::~nsCategoryImp()
{
}

nsresult nsCategoryImp::Get( PRUnichar aChar, nsUGenCategory* oResult)
{
   PRUint8 ret = GetCat(aChar);
   if( 0 == ret)
      *oResult = kUGenCategory_Other; 
   else 
      *oResult = (nsUGenCategory)ret;
   return NS_OK;
}
    
nsresult nsCategoryImp::Is( PRUnichar aChar, nsUGenCategory aCategory, PRBool* oResult)

{
   nsUGenCategory cat ;
   PRUint8 ret = GetCat(aChar);
   if( 0 == ret)
      cat = kUGenCategory_Other; 
   else 
      cat = (nsUGenCategory)ret;
   *oResult = (aCategory == cat );
   return NS_OK;
}
