










































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
#include "prprf.h"

static const size_t kNodeInfoPoolSizes[] = {
  sizeof(nsNodeInfo)
};

static const PRInt32 kNodeInfoPoolInitialSize = 
  (NS_SIZE_IN_HEAP(sizeof(nsNodeInfo))) * 64;


nsFixedSizeAllocator* nsNodeInfo::sNodeInfoPool = nsnull;


nsNodeInfo*
nsNodeInfo::Create(nsIAtom *aName, nsIAtom *aPrefix, PRInt32 aNamespaceID,
                   nsNodeInfoManager *aOwnerManager)
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
  return place ?
    new (place) nsNodeInfo(aName, aPrefix, aNamespaceID, aOwnerManager) :
    nsnull;
}

nsNodeInfo::~nsNodeInfo()
{
  mOwnerManager->RemoveNodeInfo(this);
  NS_RELEASE(mOwnerManager);

  NS_RELEASE(mInner.mName);
  NS_IF_RELEASE(mInner.mPrefix);
}


nsNodeInfo::nsNodeInfo(nsIAtom *aName, nsIAtom *aPrefix, PRInt32 aNamespaceID,
                       nsNodeInfoManager *aOwnerManager)
{
  NS_ABORT_IF_FALSE(aName, "Must have a name");
  NS_ABORT_IF_FALSE(aOwnerManager, "Must have an owner manager");

  mInner.mName = aName;
  NS_ADDREF(mInner.mName);

  mInner.mPrefix = aPrefix;
  NS_IF_ADDREF(mInner.mPrefix);

  mInner.mNamespaceID = aNamespaceID;

  mOwnerManager = aOwnerManager;
  NS_ADDREF(mOwnerManager);
}




NS_IMPL_CYCLE_COLLECTION_CLASS(nsNodeInfo)
NS_IMPL_CYCLE_COLLECTION_UNLINK_0(nsNodeInfo)

static const char* kNSURIs[] = {
  " ([none])",
  " (xmlns)",
  " (xml)",
  " (xhtml)",
  " (XLink)",
  " (XSLT)",
  " (XBL)",
  " (MathML)",
  " (RDF)",
  " (XUL)"
};

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INTERNAL(nsNodeInfo)
  if (NS_UNLIKELY(cb.WantDebugInfo())) {
    char name[72];
    PRUint32 nsid = tmp->NamespaceID();
    nsAtomCString localName(tmp->NameAtom());
    if (nsid < NS_ARRAY_LENGTH(kNSURIs)) {
      PR_snprintf(name, sizeof(name), "nsNodeInfo%s %s", kNSURIs[nsid],
                  localName.get());
    }
    else {
      PR_snprintf(name, sizeof(name), "nsNodeInfo %s", localName.get());
    }

    cb.DescribeNode(RefCounted, tmp->mRefCnt.get(), sizeof(nsNodeInfo), name);
  }
  else {
    cb.DescribeNode(RefCounted, tmp->mRefCnt.get(), sizeof(nsNodeInfo),
                    "nsNodeInfo");
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_MEMBER(mOwnerManager,
                                                  nsNodeInfoManager)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsNodeInfo)
NS_IMPL_CYCLE_COLLECTING_RELEASE_WITH_DESTROY(nsNodeInfo, LastRelease())
NS_INTERFACE_TABLE_HEAD(nsNodeInfo)
  NS_INTERFACE_TABLE1(nsNodeInfo, nsINodeInfo)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(nsNodeInfo)
NS_INTERFACE_MAP_END



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
nsNodeInfo::QualifiedNameEqualsInternal(const nsAString& aQualifiedName) const
{
  NS_PRECONDITION(mInner.mPrefix, "Must have prefix");
  
  nsAString::const_iterator start;
  aQualifiedName.BeginReading(start);

  nsAString::const_iterator colon(start);

  nsDependentAtomString prefix(mInner.mPrefix);

  if (prefix.Length() >= aQualifiedName.Length()) {
    return PR_FALSE;
  }

  colon.advance(prefix.Length());

  
  
  if (*colon != ':') {
    return PR_FALSE;
  }

  
  if (!prefix.Equals(Substring(start, colon)))
    return PR_FALSE;

  ++colon; 

  nsAString::const_iterator end;
  aQualifiedName.EndReading(end);

  
  
  return mInner.mName->Equals(Substring(colon, end));
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
