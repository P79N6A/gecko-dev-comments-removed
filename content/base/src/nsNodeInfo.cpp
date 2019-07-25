










































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
                   PRUint16 aNodeType, nsIAtom *aExtraName,
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
    new (place) nsNodeInfo(aName, aPrefix, aNamespaceID, aNodeType, aExtraName,
                           aOwnerManager) :
    nsnull;
}

nsNodeInfo::~nsNodeInfo()
{
  mOwnerManager->RemoveNodeInfo(this);
  NS_RELEASE(mOwnerManager);

  NS_RELEASE(mInner.mName);
  NS_IF_RELEASE(mInner.mPrefix);
  NS_IF_RELEASE(mInner.mExtraName);
}


nsNodeInfo::nsNodeInfo(nsIAtom *aName, nsIAtom *aPrefix, PRInt32 aNamespaceID,
                       PRUint16 aNodeType, nsIAtom* aExtraName,
                       nsNodeInfoManager *aOwnerManager)
{
  CHECK_VALID_NODEINFO(aNodeType, aName, aNamespaceID, aExtraName);
  NS_ABORT_IF_FALSE(aOwnerManager, "Invalid aOwnerManager");

  
  NS_ADDREF(mInner.mName = aName);
  NS_IF_ADDREF(mInner.mPrefix = aPrefix);
  mInner.mNamespaceID = aNamespaceID;
  mInner.mNodeType = aNodeType;
  NS_ADDREF(mOwnerManager = aOwnerManager);
  NS_IF_ADDREF(mInner.mExtraName = aExtraName);

  

  
  
  if (aPrefix) {
    mQualifiedName = nsDependentAtomString(mInner.mPrefix) +
                     NS_LITERAL_STRING(":") +
                     nsDependentAtomString(mInner.mName);
  } else {
    mInner.mName->ToString(mQualifiedName);
  }

  switch (aNodeType) {
    case nsIDOMNode::ELEMENT_NODE:
    case nsIDOMNode::ATTRIBUTE_NODE:
      
      if (aNodeType == nsIDOMNode::ELEMENT_NODE &&
          aNamespaceID == kNameSpaceID_XHTML && GetDocument() &&
          GetDocument()->IsHTML()) {
        nsContentUtils::ASCIIToUpper(mQualifiedName, mNodeName);
      } else {
        mNodeName = mQualifiedName;
      }
      mInner.mName->ToString(mLocalName);
      break;
    case nsIDOMNode::TEXT_NODE:
    case nsIDOMNode::CDATA_SECTION_NODE:
    case nsIDOMNode::COMMENT_NODE:
    case nsIDOMNode::DOCUMENT_NODE:
    case nsIDOMNode::DOCUMENT_FRAGMENT_NODE:
      mInner.mName->ToString(mNodeName);
      SetDOMStringToNull(mLocalName);
      break;
    case nsIDOMNode::PROCESSING_INSTRUCTION_NODE:
    case nsIDOMNode::DOCUMENT_TYPE_NODE:
      mInner.mExtraName->ToString(mNodeName);
      SetDOMStringToNull(mLocalName);
      break;
    default:
      NS_ABORT_IF_FALSE(aNodeType == PR_UINT16_MAX,
                        "Unknown node type");
  }
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
nsNodeInfo::NamespaceEquals(const nsAString& aNamespaceURI) const
{
  PRInt32 nsid =
    nsContentUtils::NameSpaceManager()->GetNameSpaceID(aNamespaceURI);

  return nsINodeInfo::NamespaceEquals(nsid);
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
