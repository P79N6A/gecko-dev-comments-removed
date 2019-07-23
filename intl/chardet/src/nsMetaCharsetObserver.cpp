



































#include "nsDeque.h"
#include "nsICharsetAlias.h"
#include "nsMetaCharsetObserver.h"
#include "nsIMetaCharsetService.h"
#include "nsIElementObserver.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsISupports.h"
#include "nsCRT.h"
#include "nsIParser.h"
#include "pratom.h"
#include "nsCharDetDll.h"
#include "nsIServiceManager.h"
#include "nsObserverBase.h"
#include "nsWeakReference.h"
#include "nsIParserService.h"
#include "nsParserCIID.h"
#include "nsMetaCharsetCID.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"

static NS_DEFINE_CID(kCharsetAliasCID, NS_CHARSETALIAS_CID);
 
static const eHTMLTags gWatchTags[] = 
{ eHTMLTag_meta,
  eHTMLTag_unknown
};


nsMetaCharsetObserver::nsMetaCharsetObserver()
{
  bMetaCharsetObserverStarted = PR_FALSE;
  nsresult res;
  mAlias = nsnull;
  nsCOMPtr<nsICharsetAlias> calias(do_GetService(kCharsetAliasCID, &res));
  if(NS_SUCCEEDED(res)) {
     mAlias = calias;
  }
}

nsMetaCharsetObserver::~nsMetaCharsetObserver()
{
}


NS_IMPL_ADDREF ( nsMetaCharsetObserver )
NS_IMPL_RELEASE ( nsMetaCharsetObserver )


NS_IMPL_QUERY_INTERFACE4(nsMetaCharsetObserver, 
                         nsIElementObserver, 
                         nsIObserver, 
                         nsIMetaCharsetService, 
                         nsISupportsWeakReference)


NS_IMETHODIMP nsMetaCharsetObserver::Notify(
                     PRUint32 aDocumentID, 
                     const PRUnichar* aTag, 
                     PRUint32 numOfAttributes, 
                     const PRUnichar* nameArray[], 
                     const PRUnichar* valueArray[])
{
  
    if(!nsDependentString(aTag).LowerCaseEqualsLiteral("meta")) 
        return NS_ERROR_ILLEGAL_VALUE;
    else
        return Notify(aDocumentID, numOfAttributes, nameArray, valueArray);
}

NS_IMETHODIMP nsMetaCharsetObserver::Notify(
                     PRUint32 aDocumentID, 
                     eHTMLTags aTag, 
                     PRUint32 numOfAttributes, 
                     const PRUnichar* nameArray[], 
                     const PRUnichar* valueArray[])
{
    if(eHTMLTag_meta != aTag) 
        return NS_ERROR_ILLEGAL_VALUE;
    else 
        return Notify(aDocumentID, numOfAttributes, nameArray, valueArray);
}

NS_IMETHODIMP nsMetaCharsetObserver::Notify(
                     PRUint32 aDocumentID, 
                     PRUint32 numOfAttributes, 
                     const PRUnichar* nameArray[], 
                     const PRUnichar* valueArray[])
{
   nsDeque keys(0);
   nsDeque values(0);
   PRUint32 i;
   for(i=0;i<numOfAttributes;i++)
   {
       keys.Push((void*)nameArray[i]);
       values.Push((void*)valueArray[i]);
   }
   return NS_OK;
}
NS_IMETHODIMP nsMetaCharsetObserver::Notify(
                     nsISupports* aDocShell,
                     nsISupports* aChannel,
                     const PRUnichar* aTag, 
                     const nsTArray<nsString>* keys, 
                     const nsTArray<nsString>* values,
                     const PRUint32 aFlags)
{
  nsresult result = NS_OK;
  
  if (!(aFlags & nsIElementObserver::IS_DOCUMENT_WRITE)) {
    if(!nsDependentString(aTag).LowerCaseEqualsLiteral("meta")) {
        result = NS_ERROR_ILLEGAL_VALUE;
    }
    else {
        result = Notify(aDocShell, aChannel, keys, values);
    }
  }
  return result;
}

#define IS_SPACE_CHARS(ch)  (ch == ' ' || ch == '\b' || ch == '\r' || ch == '\n')

NS_IMETHODIMP nsMetaCharsetObserver::Notify(
                    nsISupports* aDocShell,
                    nsISupports* aChannel,
                    const nsTArray<nsString>* keys, 
                    const nsTArray<nsString>* values)
{
    NS_PRECONDITION(keys!=nsnull && values!=nsnull,"Need key-value pair");

    PRUint32 numOfAttributes = keys->Length();
    NS_ASSERTION( numOfAttributes == values->Length(), "size mismatch");
    nsresult res=NS_OK;
#ifdef DEBUG

    PRUnichar Uxcommand[]={'X','_','C','O','M','M','A','N','D','\0'};
    PRUnichar UcharsetSource[]={'c','h','a','r','s','e','t','S','o','u','r','c','e','\0'};
    PRUnichar Ucharset[]={'c','h','a','r','s','e','t','\0'};
    
    NS_ASSERTION(numOfAttributes >= 3, "should have at least 3 private attribute");
    NS_ASSERTION(0==nsCRT::strcmp(Uxcommand,(keys->ElementAt(numOfAttributes-1)).get()),"last name should be 'X_COMMAND'" );
    NS_ASSERTION(0==nsCRT::strcmp(UcharsetSource,(keys->ElementAt(numOfAttributes-2)).get()),"2nd last name should be 'charsetSource'" );
    NS_ASSERTION(0==nsCRT::strcmp(Ucharset,(keys->ElementAt(numOfAttributes-3)).get()),"3rd last name should be 'charset'" );

#endif
    NS_ASSERTION(mAlias, "Didn't get nsICharsetAlias in constructor");

    if(nsnull == mAlias)
      return NS_ERROR_ABORT;

    
    if(numOfAttributes >= 5 ) 
    {
      const nsString& srcStr =  values->ElementAt(numOfAttributes-2);
      PRInt32 err;
      PRInt32  src = srcStr.ToInteger(&err);
      
      NS_ASSERTION(NS_SUCCEEDED(err), "cannot get charset source");
      if(NS_FAILED(err))
          return NS_ERROR_ILLEGAL_VALUE;

      if(kCharsetFromMetaTag <= src)
          return NS_OK; 

      const PRUnichar *httpEquivValue=nsnull;
      const PRUnichar *contentValue=nsnull;
      const PRUnichar *charsetValue=nsnull;

      for (PRUint32 i = 0; i < numOfAttributes - 3; i++)
      {
        const PRUnichar *keyStr;
        keyStr = keys->ElementAt(i).get();

        
        
        while(IS_SPACE_CHARS(*keyStr)) 
          keyStr++;

        if(Substring(keyStr, keyStr+10).LowerCaseEqualsLiteral("http-equiv"))
              httpEquivValue = values->ElementAt(i).get();
        else if(Substring(keyStr, keyStr+7).LowerCaseEqualsLiteral("content"))
              contentValue = values->ElementAt(i).get();
        else if (Substring(keyStr, keyStr+7).LowerCaseEqualsLiteral("charset"))
              charsetValue = values->ElementAt(i).get();
      }
      NS_NAMED_LITERAL_STRING(contenttype, "Content-Type");
      NS_NAMED_LITERAL_STRING(texthtml, "text/html");

      if(nsnull == httpEquivValue || nsnull == contentValue)
        return NS_OK;

      while(IS_SPACE_CHARS(*httpEquivValue))
        ++httpEquivValue;
      
      if (*httpEquivValue == '\'' || *httpEquivValue == '\"')
        ++httpEquivValue;

      while(IS_SPACE_CHARS(*contentValue))
        ++contentValue;
      
      if (*contentValue == '\'' || *contentValue == '\"')
        ++contentValue;

      if(
         Substring(httpEquivValue,
                   httpEquivValue+contenttype.Length()).Equals(contenttype,
                                                               nsCaseInsensitiveStringComparator())
         &&
         Substring(contentValue,
                   contentValue+texthtml.Length()).Equals(texthtml,
                                                          nsCaseInsensitiveStringComparator())
        )
      {

         nsCAutoString newCharset;

         if (nsnull == charsetValue) 
         {
           nsAutoString contentPart1(contentValue+9); 
           PRInt32 start = contentPart1.RFind("charset=", PR_TRUE ) ;
           PRInt32 end = contentPart1.Length();
           if(kNotFound != start)
           {
             start += 8; 
             while (start < end && contentPart1.CharAt(start) == PRUnichar(' '))
               ++start;
             if (start < end) {
               end = contentPart1.FindCharInSet("\'\"; ", start);
               if(kNotFound == end ) 
                 end = contentPart1.Length();
               NS_ASSERTION(end>=start, "wrong index");
               LossyCopyUTF16toASCII(Substring(contentPart1, start, end-start),
                                     newCharset);
             }
           } 
         }
         else   
         {
             LossyCopyUTF16toASCII(nsDependentString(charsetValue), newCharset);
         } 

         nsCAutoString charsetString;
         charsetString.AssignWithConversion(values->ElementAt(numOfAttributes-3));
         
         if (!newCharset.IsEmpty())
         {    
             if(! newCharset.Equals(charsetString, nsCaseInsensitiveCStringComparator()))
             {
                 PRBool same = PR_FALSE;
                 nsresult res2 = mAlias->Equals( newCharset, charsetString , &same);
                 if(NS_SUCCEEDED(res2) && (! same))
                 {
                     nsCAutoString preferred;
                     res2 = mAlias->GetPreferred(newCharset, preferred);
                     if(NS_SUCCEEDED(res2))
                     {
                        
                        if (!preferred.EqualsLiteral("UTF-16") &&
                            !preferred.EqualsLiteral("UTF-16BE") &&
                            !preferred.EqualsLiteral("UTF-16LE") &&
                            !preferred.EqualsLiteral("UTF-32") &&
                            !preferred.EqualsLiteral("UTF-32BE") &&
                            !preferred.EqualsLiteral("UTF-32LE")) {
                          
                          
                          res = NotifyDocShell(aDocShell,
                                               aChannel,
                                               preferred.get(),
                                               kCharsetFromMetaTag);
                        }
                     } 
                 }
             }
             else {
               res = NS_HTMLPARSER_VALID_META_CHARSET;
             } 
         } 
      } 
    }
    else
    {
      nsAutoString compatCharset;
      if (NS_SUCCEEDED(GetCharsetFromCompatibilityTag(keys, values, compatCharset)))
      {
        if (!compatCharset.IsEmpty()) {
          res = NotifyDocShell(aDocShell,
                               aChannel,
                               NS_ConvertUTF16toUTF8(compatCharset).get(), 
                               kCharsetFromMetaTag);
        }
      }
    }
    return res;
}


NS_IMETHODIMP nsMetaCharsetObserver::GetCharsetFromCompatibilityTag(
                     const nsTArray<nsString>* keys, 
                     const nsTArray<nsString>* values, 
                     nsAString& aCharset)
{
    if (!mAlias)
        return NS_ERROR_ABORT;

    aCharset.Truncate(0);
    nsresult res = NS_OK;


    
    
    PRUint32 numOfAttributes = keys->Length();
    if ((numOfAttributes >= 3) &&
        (keys->ElementAt(0).LowerCaseEqualsLiteral("charset")))
    {
      const nsString& srcStr = values->ElementAt(numOfAttributes-2);
      PRInt32 err;
      PRInt32  src = srcStr.ToInteger(&err);
      
      if (NS_FAILED(err))
          return NS_ERROR_ILLEGAL_VALUE;
      
      
      if (kCharsetFromMetaTag > src)
      {
          nsCAutoString newCharset;
          newCharset.AssignWithConversion(values->ElementAt(0).get());
          
          nsCAutoString preferred;
          res = mAlias->GetPreferred(newCharset,
                                     preferred);
          if (NS_SUCCEEDED(res))
          {
              
              
              
              const nsString& currentCharset = values->ElementAt(numOfAttributes-3);
              if (!preferred.Equals(NS_LossyConvertUTF16toASCII(currentCharset)) &&
                  !preferred.EqualsLiteral("UTF-16") &&
                  !preferred.EqualsLiteral("UTF-16BE") &&
                  !preferred.EqualsLiteral("UTF-16LE") &&
                  !preferred.EqualsLiteral("UTF-32") &&
                  !preferred.EqualsLiteral("UTF-32BE") &&
                  !preferred.EqualsLiteral("UTF-32LE"))
                  AppendASCIItoUTF16(preferred, aCharset);
          }
      }
    }

  return res;
}


NS_IMETHODIMP nsMetaCharsetObserver::Observe(nsISupports *aSubject,
                            const char *aTopic,
                               const PRUnichar *aData) 
{
  nsresult rv = NS_OK;
  if (!nsCRT::strcmp(aTopic, "parser-service-start")) {
    rv = Start();
  }
  return rv;
}


NS_IMETHODIMP nsMetaCharsetObserver::Start() 
{
  nsresult rv = NS_OK;

  if (!bMetaCharsetObserverStarted)  {
    bMetaCharsetObserverStarted = PR_TRUE;

    nsCOMPtr<nsIParserService> parserService(do_GetService(NS_PARSERSERVICE_CONTRACTID, &rv));

    if (NS_FAILED(rv))
      return rv;

    rv = parserService->RegisterObserver(this,
                                         NS_LITERAL_STRING("text/html"),
                                         gWatchTags);
  }

  return rv;
}

NS_IMETHODIMP nsMetaCharsetObserver::End() 
{
  nsresult rv = NS_OK;
  if (bMetaCharsetObserverStarted)  {
    bMetaCharsetObserverStarted = PR_FALSE;

    nsCOMPtr<nsIParserService> parserService(do_GetService(NS_PARSERSERVICE_CONTRACTID, &rv));

    if (NS_FAILED(rv))
      return rv;
    
    rv = parserService->UnregisterObserver(this, NS_LITERAL_STRING("text/html"));
  }
  return rv;
}





