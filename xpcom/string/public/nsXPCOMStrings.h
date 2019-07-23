




































#ifndef nsXPCOMStrings_h__
#define nsXPCOMStrings_h__

#include <string.h>
#include "nscore.h"










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
# define NS_CStringContainerInit          NS_CStringContainerInit_P
# define NS_CStringContainerInit2         NS_CStringContainerInit2_P
# define NS_CStringContainerFinish        NS_CStringContainerFinish_P
# define NS_CStringGetData                NS_CStringGetData_P
# define NS_CStringGetMutableData         NS_CStringGetMutableData_P
# define NS_CStringCloneData              NS_CStringCloneData_P
# define NS_CStringSetData                NS_CStringSetData_P
# define NS_CStringSetDataRange           NS_CStringSetDataRange_P
# define NS_CStringCopy                   NS_CStringCopy_P
# define NS_CStringToUTF16                NS_CStringToUTF16_P
# define NS_UTF16ToCString                NS_UTF16ToCString_P
#endif

#include "nscore.h"


class nsAString;
class nsACString;



































































class nsStringContainer;

struct nsStringContainer_base
{
private:
  void *v;
  void *d1;
  PRUint32 d2;
  void *d3;
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
  (nsStringContainer &aContainer, const PRUnichar *aData = nsnull,
   PRUint32 aDataLength = PR_UINT32_MAX, PRUint32 aFlags = 0);










XPCOM_API(void)
NS_StringContainerFinish(nsStringContainer &aContainer);




















XPCOM_API(PRUint32)
NS_StringGetData
  (const nsAString &aStr, const PRUnichar **aData,
   PRBool *aTerminated = nsnull);






























XPCOM_API(PRUint32)
NS_StringGetMutableData
  (nsAString &aStr, PRUint32 aDataLength, PRUnichar **aData);













XPCOM_API(PRUnichar *)
NS_StringCloneData
  (const nsAString &aStr);




















XPCOM_API(nsresult)
NS_StringSetData
  (nsAString &aStr, const PRUnichar *aData,
   PRUint32 aDataLength = PR_UINT32_MAX);





























XPCOM_API(nsresult)
NS_StringSetDataRange
  (nsAString &aStr, PRUint32 aCutOffset, PRUint32 aCutLength,
   const PRUnichar *aData, PRUint32 aDataLength = PR_UINT32_MAX);


















XPCOM_API(nsresult)
NS_StringCopy
  (nsAString &aDestStr, const nsAString &aSrcStr);

















inline NS_HIDDEN_(nsresult)
NS_StringAppendData(nsAString &aStr, const PRUnichar *aData,
                    PRUint32 aDataLength = PR_UINT32_MAX)
{
  return NS_StringSetDataRange(aStr, PR_UINT32_MAX, 0, aData, aDataLength);
}



















inline NS_HIDDEN_(nsresult)
NS_StringInsertData(nsAString &aStr, PRUint32 aOffset, const PRUnichar *aData,
                    PRUint32 aDataLength = PR_UINT32_MAX)
{
  return NS_StringSetDataRange(aStr, aOffset, 0, aData, aDataLength);
}












inline NS_HIDDEN_(nsresult)
NS_StringCutData(nsAString &aStr, PRUint32 aCutOffset, PRUint32 aCutLength)
{
  return NS_StringSetDataRange(aStr, aCutOffset, aCutLength, nsnull, 0);
}















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
  (nsCStringContainer &aContainer, const char *aData = nsnull,
   PRUint32 aDataLength = PR_UINT32_MAX, PRUint32 aFlags = 0);










XPCOM_API(void)
NS_CStringContainerFinish(nsCStringContainer &aContainer);




















XPCOM_API(PRUint32)
NS_CStringGetData
  (const nsACString &aStr, const char **aData,
   PRBool *aTerminated = nsnull);






























XPCOM_API(PRUint32)
NS_CStringGetMutableData
  (nsACString &aStr, PRUint32 aDataLength, char **aData);













XPCOM_API(char *)
NS_CStringCloneData
  (const nsACString &aStr);




















XPCOM_API(nsresult)
NS_CStringSetData
  (nsACString &aStr, const char *aData,
   PRUint32 aDataLength = PR_UINT32_MAX);





























XPCOM_API(nsresult)
NS_CStringSetDataRange
  (nsACString &aStr, PRUint32 aCutOffset, PRUint32 aCutLength,
   const char *aData, PRUint32 aDataLength = PR_UINT32_MAX);


















XPCOM_API(nsresult)
NS_CStringCopy
  (nsACString &aDestStr, const nsACString &aSrcStr);

















inline NS_HIDDEN_(nsresult)
NS_CStringAppendData(nsACString &aStr, const char *aData,
                    PRUint32 aDataLength = PR_UINT32_MAX)
{
  return NS_CStringSetDataRange(aStr, PR_UINT32_MAX, 0, aData, aDataLength);
}



















inline NS_HIDDEN_(nsresult)
NS_CStringInsertData(nsACString &aStr, PRUint32 aOffset, const char *aData,
                    PRUint32 aDataLength = PR_UINT32_MAX)
{
  return NS_CStringSetDataRange(aStr, aOffset, 0, aData, aDataLength);
}












inline NS_HIDDEN_(nsresult)
NS_CStringCutData(nsACString &aStr, PRUint32 aCutOffset, PRUint32 aCutLength)
{
  return NS_CStringSetDataRange(aStr, aCutOffset, aCutLength, nsnull, 0);
}






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
