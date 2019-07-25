





































#ifndef RegistryMessageUtils_h__
#define RegistryMessageUtils_h__

#include "IPC/IPCMessageUtils.h"
#include "nsIURI.h"
#include "nsNetUtil.h"

#ifndef MOZILLA_INTERNAL_API
#include "nsStringAPI.h"
#else
#include "nsString.h"
#endif

namespace IPC {

inline void WriteURI(Message* aMsg, nsIURI* aURI)
{
  nsCString spec;
  if (aURI)
    aURI->GetSpec(spec);
  WriteParam(aMsg, spec);
}

inline bool ReadURI(const Message* aMsg, void** aIter, nsIURI* *aURI)
{
  *aURI = nsnull;
  
  nsCString spec;
  if (!ReadParam(aMsg, aIter, &spec))
    return false;

  if (spec.Length()) {
    nsresult rv = NS_NewURI(aURI, spec);
    NS_ENSURE_SUCCESS(rv, false);
  }

  return true;
}
  
template <>
struct ParamTraits<ChromePackage>
{
  typedef ChromePackage paramType;
  
  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, aParam.package);
    WriteURI(aMsg, aParam.baseURI);
    WriteParam(aMsg, aParam.flags);
  }
  
  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    nsCString package;
    nsCOMPtr<nsIURI> uri;
    PRUint32 flags;
    
    if (ReadParam(aMsg, aIter, &package) &&
        ReadURI(aMsg, aIter, getter_AddRefs(uri)) &&
        ReadParam(aMsg, aIter, &flags)) {
      aResult->package = package;
      aResult->baseURI = uri;
      aResult->flags = flags;
      return true;
    }
    return false;
  }

  static void Log(const paramType& aParam, std::wstring* aLog)
  {
    nsCString spec;
    aParam.baseURI->GetSpec(spec);
    aLog->append(StringPrintf(L"[%s, %s, %u]", aParam.package.get(),
                             spec.get(), aParam.flags));
  }
};

template <>
struct ParamTraits<ChromeResource>
{
  typedef ChromeResource paramType;
  
  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, aParam.package);
    WriteURI(aMsg, aParam.resolvedURI);
  }
  
  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    nsCString package;
    nsCOMPtr<nsIURI> uri;
    
    if (ReadParam(aMsg, aIter, &package) &&
        ReadURI(aMsg, aIter, getter_AddRefs(uri))) {
      aResult->package = package;
      aResult->resolvedURI = uri;
      return true;
    }
    return false;
  }

  static void Log(const paramType& aParam, std::wstring* aLog)
  {
    nsCString spec;
    aParam.resolvedURI->GetSpec(spec);
    aLog->append(StringPrintf(L"[%s, %s, %u]", aParam.package.get(),
                             spec.get()));
  }
};

}

#endif 
