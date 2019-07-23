











































#ifndef nsIElementObserver_h__
#define nsIElementObserver_h__

#include "nsISupports.h"
#include "prtypes.h"
#include "nsHTMLTags.h"
#include "nsVoidArray.h"



#define NS_IELEMENTOBSERVER_IID      \
{ 0x4672aa04, 0xf6ae, 0x11d2, { 0xb3, 0xb7, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


class nsIElementObserver : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IELEMENTOBSERVER_IID)

  enum { IS_DOCUMENT_WRITE = 1U };
  







  NS_IMETHOD Notify(PRUint32 aDocumentID, eHTMLTags aTag, 
                    PRUint32 numOfAttributes, const PRUnichar* nameArray[], 
                    const PRUnichar* valueArray[]) = 0;

  NS_IMETHOD Notify(PRUint32 aDocumentID, const PRUnichar* aTag, 
                    PRUint32 numOfAttributes, const PRUnichar* nameArray[], 
                    const PRUnichar* valueArray[]) = 0;
  
  NS_IMETHOD Notify(nsISupports* aWebShell, 
                    nsISupports* aChannel,
                    const PRUnichar* aTag, 
                    const nsStringArray* aKeys, 
                    const nsStringArray* aValues,
                    const PRUint32 aFlags) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIElementObserver, NS_IELEMENTOBSERVER_IID)

#define NS_HTMLPARSER_VALID_META_CHARSET NS_ERROR_GENERATE_SUCCESS( \
                                          NS_ERROR_MODULE_HTMLPARSER,3000)

#endif 

