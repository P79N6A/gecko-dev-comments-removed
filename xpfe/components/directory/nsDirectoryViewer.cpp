
















#include "nsDirectoryViewer.h"
#include "nsIDirIndex.h"
#include "nsIDocShell.h"
#include "jsapi.h"
#include "nsCOMPtr.h"
#include "nsEnumeratorUtils.h"
#include "nsEscape.h"
#include "nsIRDFService.h"
#include "nsRDFCID.h"
#include "rdf.h"
#include "nsIServiceManager.h"
#include "nsISupportsArray.h"
#include "nsIXPConnect.h"
#include "nsEnumeratorUtils.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsITextToSubURI.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIFTPChannel.h"
#include "nsIWindowWatcher.h"
#include "nsIPrompt.h"
#include "nsIAuthPrompt.h"
#include "nsIProgressEventSink.h"
#include "nsIDOMWindow.h"
#include "nsIDOMWindowCollection.h"
#include "nsIDOMElement.h"
#include "nsIStreamConverterService.h"
#include "nsICategoryManager.h"
#include "nsXPCOMCID.h"
#include "nsIDocument.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/ScriptSettings.h"
#include "nsContentUtils.h"

using namespace mozilla;

static const int FORMAT_XUL = 3;






static NS_DEFINE_CID(kRDFServiceCID,             NS_RDFSERVICE_CID);


static const char               kFTPProtocol[] = "ftp://";






NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsHTTPIndex)
    NS_INTERFACE_MAP_ENTRY(nsIHTTPIndex)
    NS_INTERFACE_MAP_ENTRY(nsIRDFDataSource)
    NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
    NS_INTERFACE_MAP_ENTRY(nsIDirIndexListener)
    NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
    NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
    NS_INTERFACE_MAP_ENTRY(nsIFTPEventSink)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIHTTPIndex)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION(nsHTTPIndex, mInner)
NS_IMPL_CYCLE_COLLECTING_ADDREF(nsHTTPIndex)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsHTTPIndex)

NS_IMETHODIMP
nsHTTPIndex::GetInterface(const nsIID &anIID, void **aResult ) 
{
    if (anIID.Equals(NS_GET_IID(nsIFTPEventSink))) {
        
        

        if (!mRequestor)
          return NS_ERROR_NO_INTERFACE;
        *aResult = static_cast<nsIFTPEventSink*>(this);
        NS_ADDREF(this);
        return NS_OK;
    }

    if (anIID.Equals(NS_GET_IID(nsIPrompt))) {
        
        if (!mRequestor) 
            return NS_ERROR_NO_INTERFACE;

        nsCOMPtr<nsIDOMWindow> aDOMWindow = do_GetInterface(mRequestor);
        if (!aDOMWindow) 
            return NS_ERROR_NO_INTERFACE;

        nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
        
        return wwatch->GetNewPrompter(aDOMWindow, (nsIPrompt**)aResult);
    }  

    if (anIID.Equals(NS_GET_IID(nsIAuthPrompt))) {
        
        if (!mRequestor) 
            return NS_ERROR_NO_INTERFACE;

        nsCOMPtr<nsIDOMWindow> aDOMWindow = do_GetInterface(mRequestor);
        if (!aDOMWindow) 
            return NS_ERROR_NO_INTERFACE;

        nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
        
        return wwatch->GetNewAuthPrompter(aDOMWindow, (nsIAuthPrompt**)aResult);
    }  

    if (anIID.Equals(NS_GET_IID(nsIProgressEventSink))) {

        if (!mRequestor) 
            return NS_ERROR_NO_INTERFACE;

        nsCOMPtr<nsIProgressEventSink> sink = do_GetInterface(mRequestor);
        if (!sink) 
            return NS_ERROR_NO_INTERFACE;
        
        *aResult = sink;
        NS_ADDREF((nsISupports*)*aResult);
        return NS_OK;
    }

    return NS_ERROR_NO_INTERFACE;
}

NS_IMETHODIMP 
nsHTTPIndex::OnFTPControlLog(bool server, const char *msg)
{
    NS_ENSURE_TRUE(mRequestor, NS_OK);

    nsCOMPtr<nsIGlobalObject> globalObject = do_GetInterface(mRequestor);
    NS_ENSURE_TRUE(globalObject, NS_OK);

    
    
    dom::AutoEntryScript aes(globalObject,
                             "nsHTTPIndex OnFTPControlLog");
    JSContext* cx = aes.cx();

    JS::Rooted<JSObject*> global(cx, JS::CurrentGlobalOrNull(cx));
    NS_ENSURE_TRUE(global, NS_OK);

    nsString unicodeMsg;
    unicodeMsg.AssignWithConversion(msg);
    JSString* jsMsgStr = JS_NewUCStringCopyZ(cx, unicodeMsg.get());
    NS_ENSURE_TRUE(jsMsgStr, NS_ERROR_OUT_OF_MEMORY);

    JS::AutoValueArray<2> params(cx);
    params[0].setBoolean(server);
    params[1].setString(jsMsgStr);

    JS::Rooted<JS::Value> val(cx);
    JS_CallFunctionName(cx,
                        global,
                        "OnFTPControlLog",
                        params,
                        &val);
    return NS_OK;
}

NS_IMETHODIMP
nsHTTPIndex::SetEncoding(const char *encoding)
{
    mEncoding = encoding;
    return(NS_OK);
}

NS_IMETHODIMP
nsHTTPIndex::GetEncoding(char **encoding)
{
  NS_PRECONDITION(encoding, "null ptr");
  if (! encoding)
    return(NS_ERROR_NULL_POINTER);
  
  *encoding = ToNewCString(mEncoding);
  if (!*encoding)
    return(NS_ERROR_OUT_OF_MEMORY);
  
  return(NS_OK);
}

NS_IMETHODIMP
nsHTTPIndex::OnStartRequest(nsIRequest *request, nsISupports* aContext)
{
  nsresult rv;

  mParser = do_CreateInstance(NS_DIRINDEXPARSER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;
  
  rv = mParser->SetEncoding(mEncoding.get());
  if (NS_FAILED(rv)) return rv;

  rv = mParser->SetListener(this);
  if (NS_FAILED(rv)) return rv;

  rv = mParser->OnStartRequest(request,aContext);
  if (NS_FAILED(rv)) return rv;

  
  
  
  if (mBindToGlobalObject && mRequestor) {
    mBindToGlobalObject = false;

    nsCOMPtr<nsIGlobalObject> globalObject = do_GetInterface(mRequestor);
    NS_ENSURE_TRUE(globalObject, NS_ERROR_FAILURE);

    
    
    dom::AutoEntryScript aes(globalObject,
                             "nsHTTPIndex set HTTPIndex property");
    JSContext* cx = aes.cx();

    JS::Rooted<JSObject*> global(cx, JS::CurrentGlobalOrNull(cx));

    
    static NS_DEFINE_CID(kXPConnectCID, NS_XPCONNECT_CID);
    nsCOMPtr<nsIXPConnect> xpc(do_GetService(kXPConnectCID, &rv));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIXPConnectJSObjectHolder> wrapper;
    rv = xpc->WrapNative(cx,
                         global,
                         static_cast<nsIHTTPIndex*>(this),
                         NS_GET_IID(nsIHTTPIndex),
                         getter_AddRefs(wrapper));

    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to xpconnect-wrap http-index");
    if (NS_FAILED(rv)) return rv;

    JS::Rooted<JSObject*> jsobj(cx, wrapper->GetJSObject());
    NS_ASSERTION(jsobj,
                 "unable to get jsobj from xpconnect wrapper");
    if (!jsobj) return NS_ERROR_UNEXPECTED;

    JS::Rooted<JS::Value> jslistener(cx, OBJECT_TO_JSVAL(jsobj));

    
    bool ok = JS_SetProperty(cx, global, "HTTPIndex", jslistener);
    NS_ASSERTION(ok, "unable to set Listener property");
    if (!ok)
      return NS_ERROR_FAILURE;
  }

  if (!aContext) {
    nsCOMPtr<nsIChannel> channel(do_QueryInterface(request));
    NS_ASSERTION(channel, "request should be a channel");

    
    channel->SetNotificationCallbacks(this);

    
    nsCOMPtr<nsIURI> uri;
    channel->GetURI(getter_AddRefs(uri));
      
    nsAutoCString entryuriC;
    uri->GetSpec(entryuriC);

    nsCOMPtr<nsIRDFResource> entry;
    rv = mDirRDF->GetResource(entryuriC, getter_AddRefs(entry));
    
    NS_ConvertUTF8toUTF16 uriUnicode(entryuriC);

    nsCOMPtr<nsIRDFLiteral> URLVal;
    rv = mDirRDF->GetLiteral(uriUnicode.get(), getter_AddRefs(URLVal));

    Assert(entry, kNC_URL, URLVal, true);
    mDirectory = do_QueryInterface(entry);
  }
  else
  {
    
    mDirectory = do_QueryInterface(aContext);
  }

  if (!mDirectory) {
      request->Cancel(NS_BINDING_ABORTED);
      return NS_BINDING_ABORTED;
  }

  
  rv = Assert(mDirectory, kNC_Loading,
                           kTrueLiteral, true);
  if (NS_FAILED(rv)) return rv;

  return NS_OK;
}


NS_IMETHODIMP
nsHTTPIndex::OnStopRequest(nsIRequest *request,
                           nsISupports* aContext,
                           nsresult aStatus)
{
  
  
  
  if (! mDirectory)
    return NS_BINDING_ABORTED;

  mParser->OnStopRequest(request,aContext,aStatus);

  nsresult rv;

  nsXPIDLCString commentStr;
  mParser->GetComment(getter_Copies(commentStr));

  nsCOMPtr<nsIRDFLiteral> comment;
  rv = mDirRDF->GetLiteral(NS_ConvertASCIItoUTF16(commentStr).get(), getter_AddRefs(comment));
  if (NS_FAILED(rv)) return rv;

  rv = Assert(mDirectory, kNC_Comment, comment, true);
  if (NS_FAILED(rv)) return rv;

  
  AddElement(mDirectory, kNC_Loading, kTrueLiteral);

  return NS_OK;
}


NS_IMETHODIMP
nsHTTPIndex::OnDataAvailable(nsIRequest *request,
                             nsISupports* aContext,
                             nsIInputStream* aStream,
                             uint64_t aSourceOffset,
                             uint32_t aCount)
{
  
  
  
  if (! mDirectory)
    return NS_BINDING_ABORTED;

  return mParser->OnDataAvailable(request, mDirectory, aStream, aSourceOffset, aCount);
}


nsresult
nsHTTPIndex::OnIndexAvailable(nsIRequest* aRequest, nsISupports *aContext,
                              nsIDirIndex* aIndex)
{
  nsCOMPtr<nsIRDFResource>	parentRes = do_QueryInterface(aContext);
  if (!parentRes) {
    NS_ERROR("Could not obtain parent resource");
    return(NS_ERROR_UNEXPECTED);
  }
  
  const char* baseStr;
  parentRes->GetValueConst(&baseStr);
  if (! baseStr) {
    NS_ERROR("Could not reconstruct base uri");
    return NS_ERROR_UNEXPECTED;
  }

  
  nsAutoCString entryuriC(baseStr);

  nsXPIDLCString filename;
  nsresult rv = aIndex->GetLocation(getter_Copies(filename));
  if (NS_FAILED(rv)) return rv;
  entryuriC.Append(filename);

  
  uint32_t type;
  rv = aIndex->GetType(&type);
  if (NS_FAILED(rv))
    return rv;

  bool isDirType = (type == nsIDirIndex::TYPE_DIRECTORY);
  if (isDirType && entryuriC.Last() != '/') {
      entryuriC.Append('/');
  }

  nsCOMPtr<nsIRDFResource> entry;
  rv = mDirRDF->GetResource(entryuriC, getter_AddRefs(entry));

  
  
  
  

  if (entry && NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIRDFLiteral> lit;
    nsString str;

    str.AssignWithConversion(entryuriC.get());

    rv = mDirRDF->GetLiteral(str.get(), getter_AddRefs(lit));

    if (NS_SUCCEEDED(rv)) {
      rv = Assert(entry, kNC_URL, lit, true);
      if (NS_FAILED(rv)) return rv;
      
      nsXPIDLString xpstr;

      
      rv = aIndex->GetDescription(getter_Copies(xpstr));
      if (NS_FAILED(rv)) return rv;
      if (xpstr.Last() == '/')
        xpstr.Truncate(xpstr.Length() - 1);

      rv = mDirRDF->GetLiteral(xpstr.get(), getter_AddRefs(lit));
      if (NS_FAILED(rv)) return rv;
      rv = Assert(entry, kNC_Description, lit, true);
      if (NS_FAILED(rv)) return rv;
      
      
      int64_t size;
      rv = aIndex->GetSize(&size);
      if (NS_FAILED(rv)) return rv;
      int64_t minus1 = UINT64_MAX;
      if (size != minus1) {
        int32_t intSize = int32_t(size);
        
        nsCOMPtr<nsIRDFInt> val;
        rv = mDirRDF->GetIntLiteral(intSize, getter_AddRefs(val));
        if (NS_FAILED(rv)) return rv;
        rv = Assert(entry, kNC_ContentLength, val, true);
        if (NS_FAILED(rv)) return rv;
      }

      
      PRTime tm;
      rv = aIndex->GetLastModified(&tm);
      if (NS_FAILED(rv)) return rv;
      if (tm != -1) {
        nsCOMPtr<nsIRDFDate> val;
        rv = mDirRDF->GetDateLiteral(tm, getter_AddRefs(val));
        if (NS_FAILED(rv)) return rv;
        rv = Assert(entry, kNC_LastModified, val, true);
      }

      
      uint32_t type;
      rv = aIndex->GetType(&type);
      switch (type) {
      case nsIDirIndex::TYPE_UNKNOWN:
        rv = mDirRDF->GetLiteral(MOZ_UTF16("UNKNOWN"), getter_AddRefs(lit));
        break;
      case nsIDirIndex::TYPE_DIRECTORY:
        rv = mDirRDF->GetLiteral(MOZ_UTF16("DIRECTORY"), getter_AddRefs(lit));
        break;
      case nsIDirIndex::TYPE_FILE:
        rv = mDirRDF->GetLiteral(MOZ_UTF16("FILE"), getter_AddRefs(lit));
        break;
      case nsIDirIndex::TYPE_SYMLINK:
        rv = mDirRDF->GetLiteral(MOZ_UTF16("SYMLINK"), getter_AddRefs(lit));
        break;
      }
      
      if (NS_FAILED(rv)) return rv;
      rv = Assert(entry, kNC_FileType, lit, true);
      if (NS_FAILED(rv)) return rv;
    }

    
    
    
    if (isDirType)
      Assert(entry, kNC_IsContainer, kTrueLiteral, true);
    else
      Assert(entry, kNC_IsContainer, kFalseLiteral, true);
    




    AddElement(parentRes, kNC_Child, entry);
  }

  return rv;
}

nsresult
nsHTTPIndex::OnInformationAvailable(nsIRequest *aRequest,
                                  nsISupports *aCtxt,
                                  const nsAString& aInfo) {
  return NS_ERROR_NOT_IMPLEMENTED;
}






nsHTTPIndex::nsHTTPIndex()
  : mBindToGlobalObject(true),
    mRequestor(nullptr)
{
}


nsHTTPIndex::nsHTTPIndex(nsIInterfaceRequestor* aRequestor)
  : mBindToGlobalObject(true),
    mRequestor(aRequestor)
{
}


nsHTTPIndex::~nsHTTPIndex()
{
  
  

    if (mTimer)
    {
        
        
        mTimer->Cancel();
        mTimer = nullptr;
    }

    mConnectionList = nullptr;
    mNodeList = nullptr;
    
    if (mDirRDF)
      {
        
        mDirRDF->UnregisterDataSource(this);
      }
}



nsresult
nsHTTPIndex::CommonInit()
{
    nsresult	rv = NS_OK;

    
    mEncoding = "ISO-8859-1";

    mDirRDF = do_GetService(kRDFServiceCID, &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF service");
    if (NS_FAILED(rv)) {
      return(rv);
    }

    mInner = do_CreateInstance("@mozilla.org/rdf/datasource;1?name=in-memory-datasource", &rv);

    if (NS_FAILED(rv))
      return rv;

    mDirRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "child"),
                         getter_AddRefs(kNC_Child));
    mDirRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "loading"),
                         getter_AddRefs(kNC_Loading));
    mDirRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Comment"),
                         getter_AddRefs(kNC_Comment));
    mDirRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "URL"),
                         getter_AddRefs(kNC_URL));
    mDirRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Name"),
                         getter_AddRefs(kNC_Description));
    mDirRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Content-Length"),
                         getter_AddRefs(kNC_ContentLength));
    mDirRDF->GetResource(NS_LITERAL_CSTRING(WEB_NAMESPACE_URI "LastModifiedDate"),
                         getter_AddRefs(kNC_LastModified));
    mDirRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Content-Type"),
                         getter_AddRefs(kNC_ContentType));
    mDirRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "File-Type"),
                         getter_AddRefs(kNC_FileType));
    mDirRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "IsContainer"),
                         getter_AddRefs(kNC_IsContainer));

    rv = mDirRDF->GetLiteral(MOZ_UTF16("true"), getter_AddRefs(kTrueLiteral));
    if (NS_FAILED(rv)) return(rv);
    rv = mDirRDF->GetLiteral(MOZ_UTF16("false"), getter_AddRefs(kFalseLiteral));
    if (NS_FAILED(rv)) return(rv);

    rv = NS_NewISupportsArray(getter_AddRefs(mConnectionList));
    if (NS_FAILED(rv)) return(rv);

    
    return rv;
}


nsresult
nsHTTPIndex::Init()
{
	nsresult	rv;

	
	mEncoding = "ISO-8859-1";

	rv = CommonInit();
	if (NS_FAILED(rv))	return(rv);

	
	rv = mDirRDF->RegisterDataSource(this, false);
	if (NS_FAILED(rv)) return(rv);

	return(NS_OK);
}



nsresult
nsHTTPIndex::Init(nsIURI* aBaseURL)
{
  NS_PRECONDITION(aBaseURL != nullptr, "null ptr");
  if (! aBaseURL)
    return NS_ERROR_NULL_POINTER;

  nsresult rv;

  rv = CommonInit();
  if (NS_FAILED(rv))	return(rv);

  

  rv = aBaseURL->GetSpec(mBaseURL);
  if (NS_FAILED(rv)) return rv;
  
  
  nsCOMPtr<nsIRDFResource> baseRes;
  mDirRDF->GetResource(mBaseURL, getter_AddRefs(baseRes));
  Assert(baseRes, kNC_IsContainer, kTrueLiteral, true);

  return NS_OK;
}



nsresult
nsHTTPIndex::Create(nsIURI* aBaseURL, nsIInterfaceRequestor* aRequestor,
                    nsIHTTPIndex** aResult)
{
  *aResult = nullptr;

  nsHTTPIndex* result = new nsHTTPIndex(aRequestor);
  if (! result)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = result->Init(aBaseURL);
  if (NS_SUCCEEDED(rv))
  {
    NS_ADDREF(result);
    *aResult = result;
  }
  else
  {
    delete result;
  }
  return rv;
}

NS_IMETHODIMP
nsHTTPIndex::GetBaseURL(char** _result)
{
  *_result = ToNewCString(mBaseURL);
  if (! *_result)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

NS_IMETHODIMP
nsHTTPIndex::GetDataSource(nsIRDFDataSource** _result)
{
  NS_ADDREF(*_result = this);
  return NS_OK;
}






void nsHTTPIndex::GetDestination(nsIRDFResource* r, nsXPIDLCString& dest) {
  
  nsCOMPtr<nsIRDFNode> node;
  
  GetTarget(r, kNC_URL, true, getter_AddRefs(node));
  nsCOMPtr<nsIRDFLiteral> url;
  
  if (node)
    url = do_QueryInterface(node);

  if (!url) {
     const char* temp;
     r->GetValueConst(&temp);
     dest.Adopt(temp ? strdup(temp) : 0);
  } else {
    const char16_t* uri;
    url->GetValueConst(&uri);
    dest.Adopt(ToNewUTF8String(nsDependentString(uri)));
  }
}



















bool
nsHTTPIndex::isWellknownContainerURI(nsIRDFResource *r)
{
  nsCOMPtr<nsIRDFNode> node;
  GetTarget(r, kNC_IsContainer, true, getter_AddRefs(node));
  if (node) {
    bool isContainerFlag;
    if (NS_SUCCEEDED(node->EqualsNode(kTrueLiteral, &isContainerFlag)))
      return isContainerFlag;
  }

  nsXPIDLCString uri;
  GetDestination(r, uri);
  return uri.get() && !strncmp(uri, kFTPProtocol, sizeof(kFTPProtocol) - 1) &&
         (uri.Last() == '/');
}


NS_IMETHODIMP
nsHTTPIndex::GetURI(char * *uri)
{
	NS_PRECONDITION(uri != nullptr, "null ptr");
	if (! uri)
		return(NS_ERROR_NULL_POINTER);

	if ((*uri = strdup("rdf:httpindex")) == nullptr)
		return(NS_ERROR_OUT_OF_MEMORY);

	return(NS_OK);
}



NS_IMETHODIMP
nsHTTPIndex::GetSource(nsIRDFResource *aProperty, nsIRDFNode *aTarget, bool aTruthValue,
			nsIRDFResource **_retval)
{
	nsresult	rv = NS_ERROR_UNEXPECTED;

	*_retval = nullptr;

	if (mInner)
	{
		rv = mInner->GetSource(aProperty, aTarget, aTruthValue, _retval);
	}
	return(rv);
}

NS_IMETHODIMP
nsHTTPIndex::GetSources(nsIRDFResource *aProperty, nsIRDFNode *aTarget, bool aTruthValue,
			nsISimpleEnumerator **_retval)
{
	nsresult	rv = NS_ERROR_UNEXPECTED;

	if (mInner)
	{
		rv = mInner->GetSources(aProperty, aTarget, aTruthValue, _retval);
	}
	else
	{
		rv = NS_NewEmptyEnumerator(_retval);
	}
	return(rv);
}

NS_IMETHODIMP
nsHTTPIndex::GetTarget(nsIRDFResource *aSource, nsIRDFResource *aProperty, bool aTruthValue,
			nsIRDFNode **_retval)
{
	nsresult	rv = NS_ERROR_UNEXPECTED;

	*_retval = nullptr;

        if ((aTruthValue) && (aProperty == kNC_Child) && isWellknownContainerURI(aSource))
	{
		
		
		NS_IF_ADDREF(aSource);
		*_retval = aSource;
		return(NS_OK);
	}

	if (mInner)
	{
		rv = mInner->GetTarget(aSource, aProperty, aTruthValue, _retval);
	}
	return(rv);
}

NS_IMETHODIMP
nsHTTPIndex::GetTargets(nsIRDFResource *aSource, nsIRDFResource *aProperty, bool aTruthValue,
			nsISimpleEnumerator **_retval)
{
	nsresult	rv = NS_ERROR_UNEXPECTED;

	if (mInner)
	{
		rv = mInner->GetTargets(aSource, aProperty, aTruthValue, _retval);
	}
	else
	{
		rv = NS_NewEmptyEnumerator(_retval);
	}

	if ((aProperty == kNC_Child) && isWellknownContainerURI(aSource))
	{
		bool		doNetworkRequest = true;
		if (NS_SUCCEEDED(rv) && (_retval))
		{
			
			
			bool hasResults;
			if (NS_SUCCEEDED((*_retval)->HasMoreElements(&hasResults)) &&
			    hasResults)
			  doNetworkRequest = false;
		}

        
        
        
		if (doNetworkRequest && mConnectionList)
		{
		    int32_t connectionIndex = mConnectionList->IndexOf(aSource);
		    if (connectionIndex < 0)
		    {
    		    
	    	    mConnectionList->AppendElement(aSource);

                
                
            	if (!mTimer)
            	{
            		mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
            		NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create a timer");
            		if (NS_SUCCEEDED(rv))
            		{
                		mTimer->InitWithFuncCallback(nsHTTPIndex::FireTimer, this, 1,
                		    nsITimer::TYPE_ONE_SHOT);
                		
                		
            		}
            	}
	    	}
		}
	}

	return(rv);
}


nsresult
nsHTTPIndex::AddElement(nsIRDFResource *parent, nsIRDFResource *prop, nsIRDFNode *child)
{
    nsresult    rv;

    if (!mNodeList)
    {
        rv = NS_NewISupportsArray(getter_AddRefs(mNodeList));
        if (NS_FAILED(rv)) return(rv);
    }

    
    mNodeList->AppendElement(parent);
    mNodeList->AppendElement(prop);
    mNodeList->AppendElement(child);

	if (!mTimer)
	{
		mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
		NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create a timer");
		if (NS_FAILED(rv))  return(rv);

		mTimer->InitWithFuncCallback(nsHTTPIndex::FireTimer, this, 1,
		    nsITimer::TYPE_ONE_SHOT);
		
		
	}

    return(NS_OK);
}

void
nsHTTPIndex::FireTimer(nsITimer* aTimer, void* aClosure)
{
  nsHTTPIndex *httpIndex = static_cast<nsHTTPIndex *>(aClosure);
  if (!httpIndex)	return;
  
  
  uint32_t    numItems = 0;
  if (httpIndex->mConnectionList)
  {
        httpIndex->mConnectionList->Count(&numItems);
        if (numItems > 0)
        {
          nsCOMPtr<nsISupports>   isupports;
          httpIndex->mConnectionList->GetElementAt((uint32_t)0, getter_AddRefs(isupports));
          httpIndex->mConnectionList->RemoveElementAt((uint32_t)0);
          
          nsCOMPtr<nsIRDFResource>    aSource;
          if (isupports)  aSource = do_QueryInterface(isupports);
          
          nsXPIDLCString uri;
          if (aSource) {
            httpIndex->GetDestination(aSource, uri);
          }
          
          if (!uri) {
            NS_ERROR("Could not reconstruct uri");
            return;
          }
          
          nsresult            rv = NS_OK;
          nsCOMPtr<nsIURI>	url;
          
          rv = NS_NewURI(getter_AddRefs(url), uri.get());
          nsCOMPtr<nsIChannel>	channel;
          if (NS_SUCCEEDED(rv) && (url)) {
            rv = NS_NewChannel(getter_AddRefs(channel),
                               url,
                               nsContentUtils::GetSystemPrincipal(),
                               nsILoadInfo::SEC_NORMAL,
                               nsIContentPolicy::TYPE_OTHER);
          }
          if (NS_SUCCEEDED(rv) && (channel)) {
            channel->SetNotificationCallbacks(httpIndex);
            rv = channel->AsyncOpen(httpIndex, aSource);
          }
        }
  }
    if (httpIndex->mNodeList)
    {
        httpIndex->mNodeList->Count(&numItems);
        if (numItems > 0)
        {
            
            numItems /=3;
            if (numItems > 10)  numItems = 10;
          
            int32_t loop;
            for (loop=0; loop<(int32_t)numItems; loop++)
            {
                nsCOMPtr<nsISupports>   isupports;
                httpIndex->mNodeList->GetElementAt((uint32_t)0, getter_AddRefs(isupports));
                httpIndex->mNodeList->RemoveElementAt((uint32_t)0);
                nsCOMPtr<nsIRDFResource>    src;
                if (isupports)  src = do_QueryInterface(isupports);
                httpIndex->mNodeList->GetElementAt((uint32_t)0, getter_AddRefs(isupports));
                httpIndex->mNodeList->RemoveElementAt((uint32_t)0);
                nsCOMPtr<nsIRDFResource>    prop;
                if (isupports)  prop = do_QueryInterface(isupports);
                
                httpIndex->mNodeList->GetElementAt((uint32_t)0, getter_AddRefs(isupports));
                httpIndex->mNodeList->RemoveElementAt((uint32_t)0);
                nsCOMPtr<nsIRDFNode>    target;
                if (isupports)  target = do_QueryInterface(isupports);
                
                if (src && prop && target)
                {
                    if (prop.get() == httpIndex->kNC_Loading)
                    {
                        httpIndex->Unassert(src, prop, target);
                    }
                    else
                    {
                        httpIndex->Assert(src, prop, target, true);
                    }
                }
            }                
        }
    }

    bool refireTimer = false;
    
    if (httpIndex->mConnectionList)
    {
        httpIndex->mConnectionList->Count(&numItems);
        if (numItems > 0)
        {
            refireTimer = true;
        }
        else
        {
            httpIndex->mConnectionList->Clear();
        }
    }
    if (httpIndex->mNodeList)
    {
        httpIndex->mNodeList->Count(&numItems);
        if (numItems > 0)
        {
            refireTimer = true;
        }
        else
        {
            httpIndex->mNodeList->Clear();
        }
    }

    
    
    httpIndex->mTimer->Cancel();
    httpIndex->mTimer = nullptr;
    
    
    
    if (refireTimer)
    {
      httpIndex->mTimer = do_CreateInstance("@mozilla.org/timer;1");
      if (httpIndex->mTimer)
      {
        httpIndex->mTimer->InitWithFuncCallback(nsHTTPIndex::FireTimer, aClosure, 10,
                                                nsITimer::TYPE_ONE_SHOT);
        
        
      }
    }
}

NS_IMETHODIMP
nsHTTPIndex::Assert(nsIRDFResource *aSource, nsIRDFResource *aProperty, nsIRDFNode *aTarget,
			bool aTruthValue)
{
	nsresult	rv = NS_ERROR_UNEXPECTED;
	if (mInner)
	{
		rv = mInner->Assert(aSource, aProperty, aTarget, aTruthValue);
	}
	return(rv);
}

NS_IMETHODIMP
nsHTTPIndex::Unassert(nsIRDFResource *aSource, nsIRDFResource *aProperty, nsIRDFNode *aTarget)
{
	nsresult	rv = NS_ERROR_UNEXPECTED;
	if (mInner)
	{
		rv = mInner->Unassert(aSource, aProperty, aTarget);
	}
	return(rv);
}

NS_IMETHODIMP
nsHTTPIndex::Change(nsIRDFResource *aSource, nsIRDFResource *aProperty,
			nsIRDFNode *aOldTarget, nsIRDFNode *aNewTarget)
{
	nsresult	rv = NS_ERROR_UNEXPECTED;
	if (mInner)
	{
		rv = mInner->Change(aSource, aProperty, aOldTarget, aNewTarget);
	}
	return(rv);
}

NS_IMETHODIMP
nsHTTPIndex::Move(nsIRDFResource *aOldSource, nsIRDFResource *aNewSource,
			nsIRDFResource *aProperty, nsIRDFNode *aTarget)
{
	nsresult	rv = NS_ERROR_UNEXPECTED;
	if (mInner)
	{
		rv = mInner->Move(aOldSource, aNewSource, aProperty, aTarget);
	}
	return(rv);
}

NS_IMETHODIMP
nsHTTPIndex::HasAssertion(nsIRDFResource *aSource, nsIRDFResource *aProperty,
			nsIRDFNode *aTarget, bool aTruthValue, bool *_retval)
{
	nsresult	rv = NS_ERROR_UNEXPECTED;
	if (mInner)
	{
		rv = mInner->HasAssertion(aSource, aProperty, aTarget, aTruthValue, _retval);
	}
	return(rv);
}

NS_IMETHODIMP
nsHTTPIndex::AddObserver(nsIRDFObserver *aObserver)
{
	nsresult	rv = NS_ERROR_UNEXPECTED;
	if (mInner)
	{
		rv = mInner->AddObserver(aObserver);
	}
	return(rv);
}

NS_IMETHODIMP
nsHTTPIndex::RemoveObserver(nsIRDFObserver *aObserver)
{
	nsresult	rv = NS_ERROR_UNEXPECTED;
	if (mInner)
	{
		rv = mInner->RemoveObserver(aObserver);
	}
	return(rv);
}

NS_IMETHODIMP 
nsHTTPIndex::HasArcIn(nsIRDFNode *aNode, nsIRDFResource *aArc, bool *result)
{
  if (!mInner) {
    *result = false;
    return NS_OK;
  }
  return mInner->HasArcIn(aNode, aArc, result);
}

NS_IMETHODIMP 
nsHTTPIndex::HasArcOut(nsIRDFResource *aSource, nsIRDFResource *aArc, bool *result)
{
    if (aArc == kNC_Child && isWellknownContainerURI(aSource)) {
      *result = true;
      return NS_OK;
    }

    if (mInner) {
      return mInner->HasArcOut(aSource, aArc, result);
    }

    *result = false;
    return NS_OK;
}

NS_IMETHODIMP
nsHTTPIndex::ArcLabelsIn(nsIRDFNode *aNode, nsISimpleEnumerator **_retval)
{
	nsresult	rv = NS_ERROR_UNEXPECTED;
	if (mInner)
	{
		rv = mInner->ArcLabelsIn(aNode, _retval);
	}
	return(rv);
}

NS_IMETHODIMP
nsHTTPIndex::ArcLabelsOut(nsIRDFResource *aSource, nsISimpleEnumerator **_retval)
{
	*_retval = nullptr;

	nsCOMPtr<nsISimpleEnumerator> child, anonArcs;
	if (isWellknownContainerURI(aSource))
	{
		NS_NewSingletonEnumerator(getter_AddRefs(child), kNC_Child);
	}

	if (mInner)
	{
		mInner->ArcLabelsOut(aSource, getter_AddRefs(anonArcs));
	}

	return NS_NewUnionEnumerator(_retval, child, anonArcs);
}

NS_IMETHODIMP
nsHTTPIndex::GetAllResources(nsISimpleEnumerator **_retval)
{
	nsresult	rv = NS_ERROR_UNEXPECTED;
	if (mInner)
	{
		rv = mInner->GetAllResources(_retval);
	}
	return(rv);
}

NS_IMETHODIMP
nsHTTPIndex::IsCommandEnabled(nsISupportsArray *aSources, nsIRDFResource *aCommand,
				nsISupportsArray *aArguments, bool *_retval)
{
	nsresult	rv = NS_ERROR_UNEXPECTED;
	if (mInner)
	{
		rv = mInner->IsCommandEnabled(aSources, aCommand, aArguments, _retval);
	}
	return(rv);
}

NS_IMETHODIMP
nsHTTPIndex::DoCommand(nsISupportsArray *aSources, nsIRDFResource *aCommand,
				nsISupportsArray *aArguments)
{
	nsresult	rv = NS_ERROR_UNEXPECTED;
	if (mInner)
	{
		rv = mInner->DoCommand(aSources, aCommand, aArguments);
	}
	return(rv);
}

NS_IMETHODIMP
nsHTTPIndex::BeginUpdateBatch()
{
        return mInner->BeginUpdateBatch();
}

NS_IMETHODIMP
nsHTTPIndex::EndUpdateBatch()
{
        return mInner->EndUpdateBatch();
}

NS_IMETHODIMP
nsHTTPIndex::GetAllCmds(nsIRDFResource *aSource, nsISimpleEnumerator **_retval)
{
	nsresult	rv = NS_ERROR_UNEXPECTED;
	if (mInner)
	{
		rv = mInner->GetAllCmds(aSource, _retval);
	}
	return(rv);
}






nsDirectoryViewerFactory::nsDirectoryViewerFactory()
{
}



nsDirectoryViewerFactory::~nsDirectoryViewerFactory()
{
}


NS_IMPL_ISUPPORTS(nsDirectoryViewerFactory, nsIDocumentLoaderFactory)



NS_IMETHODIMP
nsDirectoryViewerFactory::CreateInstance(const char *aCommand,
                                         nsIChannel* aChannel,
                                         nsILoadGroup* aLoadGroup,
                                         const nsACString& aContentType, 
                                         nsIDocShell* aContainer,
                                         nsISupports* aExtraInfo,
                                         nsIStreamListener** aDocListenerResult,
                                         nsIContentViewer** aDocViewerResult)
{
  nsresult rv;

  bool viewSource = FindInReadable(NS_LITERAL_CSTRING("view-source"),
                                   aContentType);

  if (!viewSource &&
      Preferences::GetInt("network.dir.format", FORMAT_XUL) == FORMAT_XUL) {
    
    (void)aChannel->SetContentType(NS_LITERAL_CSTRING("application/vnd.mozilla.xul+xml"));

    
    
    
    
    
    nsCOMPtr<nsICategoryManager> catMan(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
    if (NS_FAILED(rv))
      return rv;
    nsXPIDLCString contractID;
    rv = catMan->GetCategoryEntry("Gecko-Content-Viewers", "application/vnd.mozilla.xul+xml",
                                  getter_Copies(contractID));
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsIDocumentLoaderFactory> factory(do_GetService(contractID, &rv));
    if (NS_FAILED(rv)) return rv;
    
    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), "chrome://communicator/content/directory/directory.xul");
    if (NS_FAILED(rv)) return rv;
    
    nsCOMPtr<nsIChannel> channel;
    rv = NS_NewChannel(getter_AddRefs(channel),
                       uri,
                       nsContentUtils::GetSystemPrincipal(),
                       nsILoadInfo::SEC_NORMAL,
                       nsIContentPolicy::TYPE_OTHER,
                       aLoadGroup);
    if (NS_FAILED(rv)) return rv;
    
    nsCOMPtr<nsIStreamListener> listener;
    rv = factory->CreateInstance(aCommand, channel, aLoadGroup,
                                 NS_LITERAL_CSTRING("application/vnd.mozilla.xul+xml"),
                                 aContainer, aExtraInfo, getter_AddRefs(listener),
                                 aDocViewerResult);
    if (NS_FAILED(rv)) return rv;

    rv = channel->AsyncOpen(listener, nullptr);
    if (NS_FAILED(rv)) return rv;
    
    
    nsCOMPtr<nsIURI> baseuri;
    rv = aChannel->GetURI(getter_AddRefs(baseuri));
    if (NS_FAILED(rv)) return rv;
    
    nsCOMPtr<nsIInterfaceRequestor> requestor = do_QueryInterface(aContainer,&rv);
    if (NS_FAILED(rv)) return rv;
    
    nsCOMPtr<nsIHTTPIndex> httpindex;
    rv = nsHTTPIndex::Create(baseuri, requestor, getter_AddRefs(httpindex));
    if (NS_FAILED(rv)) return rv;
    
    
    
    listener = do_QueryInterface(httpindex,&rv);
    *aDocListenerResult = listener.get();
    NS_ADDREF(*aDocListenerResult);
    
    return NS_OK;
  }

  
  (void)aChannel->SetContentType(NS_LITERAL_CSTRING("text/html"));

  
  nsCOMPtr<nsICategoryManager> catMan(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return rv;
  nsXPIDLCString contractID;
  rv = catMan->GetCategoryEntry("Gecko-Content-Viewers", "text/html",
                                getter_Copies(contractID));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIDocumentLoaderFactory> factory(do_GetService(contractID, &rv));
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr<nsIStreamListener> listener;

  if (viewSource) {
    rv = factory->CreateInstance("view-source", aChannel, aLoadGroup,
                                 NS_LITERAL_CSTRING("text/html; x-view-type=view-source"),
                                 aContainer, aExtraInfo, getter_AddRefs(listener),
                                 aDocViewerResult);
  } else {
    rv = factory->CreateInstance("view", aChannel, aLoadGroup,
                                 NS_LITERAL_CSTRING("text/html"),
                                 aContainer, aExtraInfo, getter_AddRefs(listener),
                                 aDocViewerResult);
  }

  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIStreamConverterService> scs = do_GetService("@mozilla.org/streamConverters;1", &rv);
  if (NS_FAILED(rv)) return rv;

  rv = scs->AsyncConvertData("application/http-index-format",
                             "text/html",
                             listener,
                             nullptr,
                             aDocListenerResult);

  if (NS_FAILED(rv)) return rv;

  return NS_OK;
}



NS_IMETHODIMP
nsDirectoryViewerFactory::CreateInstanceForDocument(nsISupports* aContainer,
                                                    nsIDocument* aDocument,
                                                    const char *aCommand,
                                                    nsIContentViewer** aDocViewerResult)
{
  NS_NOTYETIMPLEMENTED("didn't expect to get here");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDirectoryViewerFactory::CreateBlankDocument(nsILoadGroup *aLoadGroup,
                                              nsIPrincipal *aPrincipal,
                                              nsIDocument **_retval) {

  NS_NOTYETIMPLEMENTED("didn't expect to get here");
  return NS_ERROR_NOT_IMPLEMENTED;
}
