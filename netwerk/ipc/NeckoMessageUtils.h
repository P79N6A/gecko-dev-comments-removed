





































#ifndef mozilla_net_NeckoMessageUtils_h
#define mozilla_net_NeckoMessageUtils_h

#include "IPC/IPCMessageUtils.h"
#include "nsStringGlue.h"
#include "nsIURI.h"
#include "nsIIPCSerializable.h"
#include "nsIClassInfo.h"
#include "nsComponentManagerUtils.h"
#include "nsNetUtil.h"

namespace IPC {





class URI {
 public:
  URI() : mURI(nsnull) {}
  URI(nsIURI* aURI) : mURI(aURI) {}
  operator nsIURI*() const { return mURI.get(); }

  friend struct ParamTraits<URI>;
  
 private:
  
  URI(URI&);
  URI& operator=(URI&);

  nsCOMPtr<nsIURI> mURI;
};
  
template<>
struct ParamTraits<URI>
{
  typedef URI paramType;
  
  static void Write(Message* aMsg, const paramType& aParam)
  {
    bool isNull = !aParam.mURI;
    WriteParam(aMsg, isNull);
    if (isNull)
      return;
    
    nsCOMPtr<nsIIPCSerializable> serializable = do_QueryInterface(aParam.mURI);
    if (!serializable) {
      nsCString scheme;
      aParam.mURI->GetScheme(scheme);
      if (!scheme.EqualsASCII("about:") &&
          !scheme.EqualsASCII("javascript:") &&
          !scheme.EqualsASCII("javascript"))
        NS_WARNING("All IPDL URIs must be serializable or an allowed scheme");
    }
    
    bool isSerialized = !!serializable;
    WriteParam(aMsg, isSerialized);
    if (!isSerialized) {
      nsCString spec, charset;
      aParam.mURI->GetSpec(spec);
      aParam.mURI->GetOriginCharset(charset);
      WriteParam(aMsg, spec);
      WriteParam(aMsg, charset);
      return;
    }
    
    nsCOMPtr<nsIClassInfo> classInfo = do_QueryInterface(aParam.mURI);
    char cidStr[NSID_LENGTH];
    nsCID cid;
    nsresult rv = classInfo->GetClassIDNoAlloc(&cid);
    NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv), "All IPDL URIs must report a valid class ID");
    
    cid.ToProvidedString(cidStr);
    WriteParam(aMsg, nsCAutoString(cidStr));
    serializable->Write(aMsg);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    bool isNull;
    if (!ReadParam(aMsg, aIter, &isNull))
      return false;
    if (isNull) {
      aResult->mURI = nsnull;
      return true;
    }

    bool isSerialized;
    if (!ReadParam(aMsg, aIter, &isSerialized))
      return false;
    if (!isSerialized) {
      nsCString spec, charset;
      if (!ReadParam(aMsg, aIter, &spec) ||
          !ReadParam(aMsg, aIter, &charset))
        return false;
      
      nsCOMPtr<nsIURI> uri;
      nsresult rv = NS_NewURI(getter_AddRefs(uri), spec, charset.get());
      if (NS_FAILED(rv))
        return false;
      
      uri.swap(aResult->mURI);
      return true;
    }
    
    nsCAutoString cidStr;
    nsCID cid;
    if (!ReadParam(aMsg, aIter, &cidStr) ||
        !cid.Parse(cidStr.get()))
      return false;

    nsCOMPtr<nsIURI> uri = do_CreateInstance(cid);
    if (!uri)
      return false;
    nsCOMPtr<nsIIPCSerializable> serializable = do_QueryInterface(uri);
    if (!serializable || !serializable->Read(aMsg, aIter))
      return false;

    uri.swap(aResult->mURI);
    return true;
  }

  static void Log(const paramType& aParam, std::wstring* aLog)
  {
    nsIURI* uri = aParam.mURI;
    if (uri) {
      nsCString spec;
      uri->GetSpec(spec);
      aLog->append(StringPrintf(L"[%s]", spec.get()));
    }
    else {
      aLog->append(L"[]");
    }
  }
};

}

#endif 
