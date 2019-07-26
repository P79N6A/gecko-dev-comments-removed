




#ifndef nsXPCOMStrings_h__
#define nsXPCOMStrings_h__

#include <string.h>
#include "nscore.h"
#include <limits>










#ifdef MOZILLA_INTERNAL_API
# define NS_StringContainerInit           NS_StringContainerInit_P
# define NS_StringContainerInit2          NS_StringContainerInit2_P
# define NS_StringContainerFinish         NS_StringContainerFinish_P
# define NS_StringGetData                 NS_StringGetData_P
# define NS_StringGetMutableData          NS_StringGetMutableData_P
# define NS_StringCloneData               NS_StringCloneData_P
# define NS_StringSetData                 NS_StringSetData_P
# define NS_StringSetDataRange            NS_StringSetDataRange_P
# define NS_StringCopy                    NS_StringCopy_P
# define NS_StringSetIsVoid               NS_StringSetIsVoid_P
# define NS_StringGetIsVoid               NS_StringGetIsVoid_P
# define NS_CStringContainerInit          NS_CStringContainerInit_P
# define NS_CStringContainerInit2         NS_CStringContainerInit2_P
# define NS_CStringContainerFinish        NS_CStringContainerFinish_P
# define NS_CStringGetData                NS_CStringGetData_P
# define NS_CStringGetMutableData         NS_CStringGetMutableData_P
# define NS_CStringCloneData              NS_CStringCloneData_P
# define NS_CStringSetData                NS_CStringSetData_P
# define NS_CStringSetDataRange           NS_CStringSetDataRange_P
# define NS_CStringCopy                   NS_CStringCopy_P
# define NS_CStringSetIsVoid              NS_CStringSetIsVoid_P
# define NS_CStringGetIsVoid              NS_CStringGetIsVoid_P
# define NS_CStringToUTF16                NS_CStringToUTF16_P
# define NS_UTF16ToCString                NS_UTF16ToCString_P
#endif

#include "nscore.h"


class nsAString;
class nsACString;



































































class nsStringContainer;












struct nsStringContainer_base
{
protected:
  void *d1;
  uint32_t d2;
  uint32_t d3;
};




enum {
  



  NS_STRING_CONTAINER_INIT_DEPEND    = (1 << 1),

  



  NS_STRING_CONTAINER_INIT_ADOPT     = (1 << 2),

  

  NS_STRING_CONTAINER_INIT_SUBSTRING = (1 << 3)
};










XPCOM_API(nsresult)
NS_StringContainerInit(nsStringContainer &aContainer);




















XPCOM_API(nsresult)
NS_StringContainerInit2
  (nsStringContainer &aContainer, const PRUnichar *aData = nullptr,
   uint32_t aDataLength = UINT32_MAX, uint32_t aFlags = 0);








XPCOM_API(void)
NS_StringContainerFinish(nsStringContainer &aContainer);


















XPCOM_API(uint32_t)
NS_StringGetData
  (const nsAString &aStr, const PRUnichar **aData,
   bool *aTerminated = nullptr);




























XPCOM_API(uint32_t)
NS_StringGetMutableData
  (nsAString &aStr, uint32_t aDataLength, PRUnichar **aData);











XPCOM_API(PRUnichar *)
NS_StringCloneData
  (const nsAString &aStr);


















XPCOM_API(nsresult)
NS_StringSetData
  (nsAString &aStr, const PRUnichar *aData,
   uint32_t aDataLength = UINT32_MAX);



























XPCOM_API(nsresult)
NS_StringSetDataRange
  (nsAString &aStr, uint32_t aCutOffset, uint32_t aCutLength,
   const PRUnichar *aData, uint32_t aDataLength = UINT32_MAX);
















XPCOM_API(nsresult)
NS_StringCopy
  (nsAString &aDestStr, const nsAString &aSrcStr);

















inline NS_HIDDEN_(nsresult)
NS_StringAppendData(nsAString &aStr, const PRUnichar *aData,
                    uint32_t aDataLength = UINT32_MAX)
{
  return NS_StringSetDataRange(aStr, UINT32_MAX, 0, aData, aDataLength);
}



















inline NS_HIDDEN_(nsresult)
NS_StringInsertData(nsAString &aStr, uint32_t aOffset, const PRUnichar *aData,
                    uint32_t aDataLength = UINT32_MAX)
{
  return NS_StringSetDataRange(aStr, aOffset, 0, aData, aDataLength);
}












inline NS_HIDDEN_(nsresult)
NS_StringCutData(nsAString &aStr, uint32_t aCutOffset, uint32_t aCutLength)
{
  return NS_StringSetDataRange(aStr, aCutOffset, aCutLength, nullptr, 0);
}







XPCOM_API(void)
NS_StringSetIsVoid(nsAString& aStr, const bool aIsVoid);







XPCOM_API(bool)
NS_StringGetIsVoid(const nsAString& aStr);















class nsCStringContainer;




enum {
  



  NS_CSTRING_CONTAINER_INIT_DEPEND    = (1 << 1),

  



  NS_CSTRING_CONTAINER_INIT_ADOPT     = (1 << 2),

  

  NS_CSTRING_CONTAINER_INIT_SUBSTRING = (1 << 3)
};










XPCOM_API(nsresult)
NS_CStringContainerInit(nsCStringContainer &aContainer);




















XPCOM_API(nsresult)
NS_CStringContainerInit2
  (nsCStringContainer &aContainer, const char *aData = nullptr,
   uint32_t aDataLength = UINT32_MAX, uint32_t aFlags = 0);








XPCOM_API(void)
NS_CStringContainerFinish(nsCStringContainer &aContainer);


















XPCOM_API(uint32_t)
NS_CStringGetData
  (const nsACString &aStr, const char **aData,
   bool *aTerminated = nullptr);




























XPCOM_API(uint32_t)
NS_CStringGetMutableData
  (nsACString &aStr, uint32_t aDataLength, char **aData);











XPCOM_API(char *)
NS_CStringCloneData
  (const nsACString &aStr);


















XPCOM_API(nsresult)
NS_CStringSetData
  (nsACString &aStr, const char *aData,
   uint32_t aDataLength = UINT32_MAX);



























XPCOM_API(nsresult)
NS_CStringSetDataRange
  (nsACString &aStr, uint32_t aCutOffset, uint32_t aCutLength,
   const char *aData, uint32_t aDataLength = UINT32_MAX);
















XPCOM_API(nsresult)
NS_CStringCopy
  (nsACString &aDestStr, const nsACString &aSrcStr);

















inline NS_HIDDEN_(nsresult)
NS_CStringAppendData(nsACString &aStr, const char *aData,
                    uint32_t aDataLength = UINT32_MAX)
{
  return NS_CStringSetDataRange(aStr, UINT32_MAX, 0, aData, aDataLength);
}



















inline NS_HIDDEN_(nsresult)
NS_CStringInsertData(nsACString &aStr, uint32_t aOffset, const char *aData,
                    uint32_t aDataLength = UINT32_MAX)
{
  return NS_CStringSetDataRange(aStr, aOffset, 0, aData, aDataLength);
}












inline NS_HIDDEN_(nsresult)
NS_CStringCutData(nsACString &aStr, uint32_t aCutOffset, uint32_t aCutLength)
{
  return NS_CStringSetDataRange(aStr, aCutOffset, aCutLength, nullptr, 0);
}







XPCOM_API(void)
NS_CStringSetIsVoid(nsACString& aStr, const bool aIsVoid);







XPCOM_API(bool)
NS_CStringGetIsVoid(const nsACString& aStr);






enum nsCStringEncoding {
  




  NS_CSTRING_ENCODING_ASCII = 0,

  
  NS_CSTRING_ENCODING_UTF8 = 1,

  



  NS_CSTRING_ENCODING_NATIVE_FILESYSTEM = 2
};












XPCOM_API(nsresult)
NS_CStringToUTF16(const nsACString &aSource, nsCStringEncoding aSrcEncoding,
                  nsAString &aDest);














XPCOM_API(nsresult)
NS_UTF16ToCString(const nsAString &aSource, nsCStringEncoding aDestEncoding,
                  nsACString &aDest);

#endif 
