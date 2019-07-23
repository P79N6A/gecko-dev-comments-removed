





































#include "nscore.h"
#include "nsLoadSaveContentSink.h"

nsresult
NS_NewLoadSaveContentSink(nsILoadSaveContentSink** aResult,
                          nsIXMLContentSink* aBaseSink)
{
  NS_PRECONDITION(aResult, "Null out ptr?  Who do you think you are, flouting XPCOM contract?");
  NS_ENSURE_ARG_POINTER(aBaseSink);
  nsLoadSaveContentSink* it;
  NS_NEWXPCOM(it, nsLoadSaveContentSink);
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  nsresult rv = it->Init(aBaseSink);
  if (NS_FAILED(rv)) {
    delete it;
    return rv;
  }

  return CallQueryInterface(it, aResult);  
}

nsLoadSaveContentSink::nsLoadSaveContentSink()
{
}

nsLoadSaveContentSink::~nsLoadSaveContentSink()
{
}

nsresult
nsLoadSaveContentSink::Init(nsIXMLContentSink* aBaseSink)
{
  NS_PRECONDITION(aBaseSink, "aBaseSink needs to exist");
  mBaseSink = aBaseSink;
  mExpatSink = do_QueryInterface(aBaseSink);
  if (!mExpatSink) {
    return NS_ERROR_INVALID_ARG;
  }

  return NS_OK;
}

NS_IMPL_THREADSAFE_ADDREF(nsLoadSaveContentSink)
NS_IMPL_THREADSAFE_RELEASE(nsLoadSaveContentSink)

NS_INTERFACE_MAP_BEGIN(nsLoadSaveContentSink)
  NS_INTERFACE_MAP_ENTRY(nsIXMLContentSink)
  NS_INTERFACE_MAP_ENTRY(nsIContentSink)
  NS_INTERFACE_MAP_ENTRY(nsIExpatSink)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXMLContentSink)
NS_INTERFACE_MAP_END


NS_IMETHODIMP
nsLoadSaveContentSink::WillBuildModel(nsDTDMode aDTDMode)
{
  return mBaseSink->WillBuildModel(aDTDMode);
}

NS_IMETHODIMP
nsLoadSaveContentSink::DidBuildModel(PRBool aTerminated)
{
  return mBaseSink->DidBuildModel(aTerminated);
}

NS_IMETHODIMP
nsLoadSaveContentSink::WillInterrupt(void)
{
  return mBaseSink->WillInterrupt();
}

NS_IMETHODIMP
nsLoadSaveContentSink::WillResume(void)
{
  return mBaseSink->WillResume();
}

NS_IMETHODIMP
nsLoadSaveContentSink::SetParser(nsIParser* aParser)
{
  return mBaseSink->SetParser(aParser);
}

void
nsLoadSaveContentSink::FlushPendingNotifications(mozFlushType aType)
{
  mBaseSink->FlushPendingNotifications(aType);
}

NS_IMETHODIMP
nsLoadSaveContentSink::SetDocumentCharset(nsAString& aCharset)
{
  return mBaseSink->SetDocumentCharset(aCharset);
}



NS_IMETHODIMP
nsLoadSaveContentSink::HandleStartElement(const PRUnichar *aName, 
                                          const PRUnichar **aAtts, 
                                          PRUint32 aAttsCount, 
                                          PRInt32 aIndex, 
                                          PRUint32 aLineNumber)
{
  return mExpatSink->HandleStartElement(aName, aAtts, aAttsCount, aIndex,
                                        aLineNumber);
}

NS_IMETHODIMP
nsLoadSaveContentSink::HandleEndElement(const PRUnichar *aName)
{
  return mExpatSink->HandleEndElement(aName);
}

NS_IMETHODIMP
nsLoadSaveContentSink::HandleComment(const PRUnichar *aName)
{
  return mExpatSink->HandleComment(aName);
}

NS_IMETHODIMP
nsLoadSaveContentSink::HandleCDataSection(const PRUnichar *aData, 
                                          PRUint32 aLength)
{
  return mExpatSink->HandleCDataSection(aData, aLength);
}

NS_IMETHODIMP
nsLoadSaveContentSink::HandleDoctypeDecl(const nsAString & aSubset, 
                                         const nsAString & aName, 
                                         const nsAString & aSystemId, 
                                         const nsAString & aPublicId,
                                         nsISupports* aCatalogData)
{
  return mExpatSink->HandleDoctypeDecl(aSubset, aName, aSystemId, aPublicId,
                                       aCatalogData);
}

NS_IMETHODIMP
nsLoadSaveContentSink::HandleCharacterData(const PRUnichar *aData, 
                                           PRUint32 aLength)
{
  return mExpatSink->HandleCharacterData(aData, aLength);
}

NS_IMETHODIMP
nsLoadSaveContentSink::HandleProcessingInstruction(const PRUnichar *aTarget, 
                                                   const PRUnichar *aData)
{
  return mExpatSink->HandleProcessingInstruction(aTarget, aData);
}

NS_IMETHODIMP
nsLoadSaveContentSink::HandleXMLDeclaration(const PRUnichar *aVersion,
                                            const PRUnichar *aEncoding,
                                            PRInt32 aStandalone)
{
  return mExpatSink->HandleXMLDeclaration(aVersion, aEncoding, aStandalone);
}

NS_IMETHODIMP
nsLoadSaveContentSink::ReportError(const PRUnichar* aErrorText, 
                                   const PRUnichar* aSourceText,
                                   nsIScriptError *aError,
                                   PRBool *_retval)
{
  NS_PRECONDITION(aError && aSourceText && aErrorText, "Check arguments!!!");

  
  
  

  
  *_retval = PR_TRUE;
  return NS_OK;
}
