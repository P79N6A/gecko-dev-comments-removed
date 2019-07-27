




#include "nsHostObjectURI.h"

#include "nsAutoPtr.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"

#include "mozilla/ipc/BackgroundUtils.h"
#include "mozilla/ipc/URIUtils.h"

static NS_DEFINE_CID(kHOSTOBJECTURICID, NS_HOSTOBJECTURI_CID);

static NS_DEFINE_CID(kThisSimpleURIImplementationCID,
                     NS_THIS_SIMPLEURI_IMPLEMENTATION_CID);

NS_IMPL_ADDREF_INHERITED(nsHostObjectURI, nsSimpleURI)
NS_IMPL_RELEASE_INHERITED(nsHostObjectURI, nsSimpleURI)

NS_INTERFACE_MAP_BEGIN(nsHostObjectURI)
  NS_INTERFACE_MAP_ENTRY(nsIURIWithPrincipal)
  if (aIID.Equals(kHOSTOBJECTURICID))
    foundInterface = static_cast<nsIURI*>(this);
  else if (aIID.Equals(kThisSimpleURIImplementationCID)) {
    
    
    
    *aInstancePtr = nullptr;
    return NS_NOINTERFACE;
  }
  else
NS_INTERFACE_MAP_END_INHERITING(nsSimpleURI)



NS_IMETHODIMP
nsHostObjectURI::GetPrincipal(nsIPrincipal** aPrincipal)
{
  NS_IF_ADDREF(*aPrincipal = mPrincipal);

  return NS_OK;
}

NS_IMETHODIMP
nsHostObjectURI::GetPrincipalUri(nsIURI** aUri)
{
  if (mPrincipal) {
    mPrincipal->GetURI(aUri);
  }
  else {
    *aUri = nullptr;
  }

  return NS_OK;
}



NS_IMETHODIMP
nsHostObjectURI::Read(nsIObjectInputStream* aStream)
{
  nsresult rv = nsSimpleURI::Read(aStream);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupports> supports;
  rv = NS_ReadOptionalObject(aStream, true, getter_AddRefs(supports));
  NS_ENSURE_SUCCESS(rv, rv);

  mPrincipal = do_QueryInterface(supports, &rv);
  return rv;
}

NS_IMETHODIMP
nsHostObjectURI::Write(nsIObjectOutputStream* aStream)
{
  nsresult rv = nsSimpleURI::Write(aStream);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_WriteOptionalCompoundObject(aStream, mPrincipal,
                                        NS_GET_IID(nsIPrincipal),
                                        true);
}


void
nsHostObjectURI::Serialize(mozilla::ipc::URIParams& aParams)
{
  using namespace mozilla::ipc;

  HostObjectURIParams hostParams;
  URIParams simpleParams;

  nsSimpleURI::Serialize(simpleParams);
  hostParams.simpleParams() = simpleParams;

  if (mPrincipal) {
    PrincipalInfo info;
    nsresult rv = PrincipalToPrincipalInfo(mPrincipal, &info);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return;
    }

    hostParams.principal() = info;
  } else {
    hostParams.principal() = void_t();
  }

  aParams = hostParams;
}

bool
nsHostObjectURI::Deserialize(const mozilla::ipc::URIParams& aParams)
{
  using namespace mozilla::ipc;

  if (aParams.type() != URIParams::THostObjectURIParams) {
      NS_ERROR("Received unknown parameters from the other process!");
      return false;
  }

  const HostObjectURIParams& hostParams = aParams.get_HostObjectURIParams();

  if (!nsSimpleURI::Deserialize(hostParams.simpleParams())) {
    return false;
  }
  if (hostParams.principal().type() == OptionalPrincipalInfo::Tvoid_t) {
    return true;
  }

  mPrincipal = PrincipalInfoToPrincipal(hostParams.principal().get_PrincipalInfo());
  return mPrincipal != nullptr;
}


nsresult
nsHostObjectURI::CloneInternal(nsSimpleURI::RefHandlingEnum aRefHandlingMode,
                               nsIURI** aClone)
{
  nsCOMPtr<nsIURI> simpleClone;
  nsresult rv =
    nsSimpleURI::CloneInternal(aRefHandlingMode, getter_AddRefs(simpleClone));
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef DEBUG
  nsRefPtr<nsHostObjectURI> uriCheck;
  rv = simpleClone->QueryInterface(kHOSTOBJECTURICID, getter_AddRefs(uriCheck));
  MOZ_ASSERT(NS_SUCCEEDED(rv) && uriCheck);
#endif

  nsHostObjectURI* u = static_cast<nsHostObjectURI*>(simpleClone.get());

  u->mPrincipal = mPrincipal;

  simpleClone.forget(aClone);
  return NS_OK;
}

 nsresult
nsHostObjectURI::EqualsInternal(nsIURI* aOther,
                                nsSimpleURI::RefHandlingEnum aRefHandlingMode,
                                bool* aResult)
{
  if (!aOther) {
    *aResult = false;
    return NS_OK;
  }
  
  nsRefPtr<nsHostObjectURI> otherUri;
  aOther->QueryInterface(kHOSTOBJECTURICID, getter_AddRefs(otherUri));
  if (!otherUri) {
    *aResult = false;
    return NS_OK;
  }

  
  if (!nsSimpleURI::EqualsInternal(otherUri, aRefHandlingMode)) {
    *aResult = false;
    return NS_OK;
  }

  
  if (mPrincipal && otherUri->mPrincipal) {
    
    return mPrincipal->Equals(otherUri->mPrincipal, aResult);
  }
  
  *aResult = (!mPrincipal && !otherUri->mPrincipal);
  return NS_OK;
}


NS_IMETHODIMP 
nsHostObjectURI::GetInterfaces(uint32_t *count, nsIID * **array)
{
  *count = 0;
  *array = nullptr;
  return NS_OK;
}

NS_IMETHODIMP 
nsHostObjectURI::GetScriptableHelper(nsIXPCScriptable **_retval)
{
  *_retval = nullptr;
  return NS_OK;
}

NS_IMETHODIMP 
nsHostObjectURI::GetContractID(char * *aContractID)
{
  
  
  *aContractID = nullptr;
  return NS_OK;
}

NS_IMETHODIMP 
nsHostObjectURI::GetClassDescription(char * *aClassDescription)
{
  *aClassDescription = nullptr;
  return NS_OK;
}

NS_IMETHODIMP 
nsHostObjectURI::GetClassID(nsCID * *aClassID)
{
  
  
  *aClassID = (nsCID*) moz_xmalloc(sizeof(nsCID));
  NS_ENSURE_TRUE(*aClassID, NS_ERROR_OUT_OF_MEMORY);

  return GetClassIDNoAlloc(*aClassID);
}

NS_IMETHODIMP 
nsHostObjectURI::GetFlags(uint32_t *aFlags)
{
  *aFlags = nsIClassInfo::MAIN_THREAD_ONLY;
  return NS_OK;
}

NS_IMETHODIMP 
nsHostObjectURI::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
  *aClassIDNoAlloc = kHOSTOBJECTURICID;
  return NS_OK;
}
