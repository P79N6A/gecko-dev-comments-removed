










































#include "nscore.h"
#include "nsNodeInfo.h"
#include "nsNodeInfoManager.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIAtom.h"
#include "nsDOMString.h"
#include "nsCRT.h"
#include "nsContentUtils.h"
#include "nsReadableUtils.h"
#include "nsAutoPtr.h"
#include NEW_H
#include "nsFixedSizeAllocator.h"

static const size_t kNodeInfoPoolSizes[] = {
  sizeof(nsNodeInfo)
};

static const PRInt32 kNodeInfoPoolInitialSize = 
  (NS_SIZE_IN_HEAP(sizeof(nsNodeInfo))) * 64;


nsFixedSizeAllocator* nsNodeInfo::sNodeInfoPool = nsnull;


nsNodeInfo*
nsNodeInfo::Create()
{
  if (!sNodeInfoPool) {
    sNodeInfoPool = new nsFixedSizeAllocator();
    if (!sNodeInfoPool)
      return nsnull;

    nsresult rv = sNodeInfoPool->Init("NodeInfo Pool", kNodeInfoPoolSizes,
                                      1, kNodeInfoPoolInitialSize);
    if (NS_FAILED(rv)) {
      delete sNodeInfoPool;
      sNodeInfoPool = nsnull;
      return nsnull;
    }
  }

  
  void* place = sNodeInfoPool->Alloc(sizeof(nsNodeInfo));
  return place ? new (place) nsNodeInfo() : nsnull;
}

nsNodeInfo::nsNodeInfo()
{
}

nsNodeInfo::~nsNodeInfo()
{
  if (mOwnerManager) {
    mOwnerManager->RemoveNodeInfo(this);
    NS_RELEASE(mOwnerManager);
  }

  NS_IF_RELEASE(mInner.mName);
  NS_IF_RELEASE(mInner.mPrefix);
}


nsresult
nsNodeInfo::Init(nsIAtom *aName, nsIAtom *aPrefix, PRInt32 aNamespaceID,
                 nsNodeInfoManager *aOwnerManager)
{
  NS_ENSURE_TRUE(!mInner.mName && !mInner.mPrefix && !mOwnerManager,
                 NS_ERROR_ALREADY_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aName);
  NS_ENSURE_ARG_POINTER(aOwnerManager);

  mInner.mName = aName;
  NS_ADDREF(mInner.mName);

  mInner.mPrefix = aPrefix;
  NS_IF_ADDREF(mInner.mPrefix);

  mInner.mNamespaceID = aNamespaceID;

  mOwnerManager = aOwnerManager;
  NS_ADDREF(mOwnerManager);

  return NS_OK;
}




NS_IMPL_ADDREF(nsNodeInfo)
NS_IMPL_RELEASE_WITH_DESTROY(nsNodeInfo, LastRelease())
NS_IMPL_QUERY_INTERFACE1(nsNodeInfo, nsINodeInfo)



void
nsNodeInfo::GetQualifiedName(nsAString& aQualifiedName) const
{
  if (mInner.mPrefix) {
    mInner.mPrefix->ToString(aQualifiedName);

    aQualifiedName.Append(PRUnichar(':'));
  } else {
    aQualifiedName.Truncate();
  }

  nsAutoString name;
  mInner.mName->ToString(name);

  aQualifiedName.Append(name);
}


void
nsNodeInfo::GetLocalName(nsAString& aLocalName) const
{
#ifdef STRICT_DOM_LEVEL2_LOCALNAME
  if (mInner.mNamespaceID > 0) {
    mInner.mName->ToString(aLocalName);
  } else {
    SetDOMStringToNull(aLocalName);
  }
#else
  mInner.mName->ToString(aLocalName);
#endif
}


nsresult
nsNodeInfo::GetNamespaceURI(nsAString& aNameSpaceURI) const
{
  nsresult rv = NS_OK;

  if (mInner.mNamespaceID > 0) {
    rv = nsContentUtils::NameSpaceManager()->GetNameSpaceURI(mInner.mNamespaceID,
                                                             aNameSpaceURI);
  } else {
    SetDOMStringToNull(aNameSpaceURI);
  }

  return rv;
}


PRBool
nsNodeInfo::Equals(const nsAString& aName) const
{
  return mInner.mName->Equals(aName);
}


PRBool
nsNodeInfo::Equals(const nsAString& aName, const nsAString& aPrefix) const
{
  if (!mInner.mName->Equals(aName)) {
    return PR_FALSE;
  }

  if (!mInner.mPrefix) {
    return aPrefix.IsEmpty();
  }

  return mInner.mPrefix->Equals(aPrefix);
}


PRBool
nsNodeInfo::Equals(const nsAString& aName, PRInt32 aNamespaceID) const
{
  return mInner.mNamespaceID == aNamespaceID &&
    mInner.mName->Equals(aName);
}


PRBool
nsNodeInfo::Equals(const nsAString& aName, const nsAString& aPrefix,
                   PRInt32 aNamespaceID) const
{
  if (!mInner.mNamespaceID == aNamespaceID ||
      !mInner.mName->Equals(aName))
    return PR_FALSE;

  return mInner.mPrefix ? mInner.mPrefix->Equals(aPrefix) :
    aPrefix.IsEmpty();
}


PRBool
nsNodeInfo::NamespaceEquals(const nsAString& aNamespaceURI) const
{
  PRInt32 nsid =
    nsContentUtils::NameSpaceManager()->GetNameSpaceID(aNamespaceURI);

  return nsINodeInfo::NamespaceEquals(nsid);
}

PRBool
nsNodeInfo::QualifiedNameEqualsInternal(const nsACString& aQualifiedName) const
{
  NS_PRECONDITION(mInner.mPrefix, "Must have prefix");
  
  nsACString::const_iterator start;
  aQualifiedName.BeginReading(start);

  nsACString::const_iterator colon(start);

  const char* prefix;
  mInner.mPrefix->GetUTF8String(&prefix);

  PRUint32 len = strlen(prefix);

  if (len >= aQualifiedName.Length()) {
    return PR_FALSE;
  }

  colon.advance(len);

  
  
  if (*colon != ':') {
    return PR_FALSE;
  }

  
  if (!mInner.mPrefix->EqualsUTF8(Substring(start, colon)))
    return PR_FALSE;

  ++colon; 

  nsACString::const_iterator end;
  aQualifiedName.EndReading(end);

  
  
  return mInner.mName->EqualsUTF8(Substring(colon, end));
}


void
nsNodeInfo::ClearCache()
{
  
  delete sNodeInfoPool;
  sNodeInfoPool = nsnull;
}

void
nsNodeInfo::LastRelease()
{
  nsRefPtr<nsNodeInfoManager> kungFuDeathGrip = mOwnerManager;
  this->~nsNodeInfo();

  
  
  
  mRefCnt = 0;

  NS_ASSERTION(sNodeInfoPool, "No NodeInfoPool when deleting NodeInfo!!!");
  sNodeInfoPool->Free(this, sizeof(nsNodeInfo));
}
