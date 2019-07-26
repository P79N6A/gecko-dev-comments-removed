










#include "mozilla/Util.h"
#include "mozilla/Likely.h"

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
#include "nsIDocument.h"
#include "nsGkAtoms.h"
#include "nsCCUncollectableMarker.h"

using namespace mozilla;

static const size_t kNodeInfoPoolSizes[] = {
  sizeof(nsNodeInfo)
};

static const int32_t kNodeInfoPoolInitialSize = sizeof(nsNodeInfo) * 64;


nsFixedSizeAllocator* nsNodeInfo::sNodeInfoPool = nullptr;


nsNodeInfo*
nsNodeInfo::Create(nsIAtom *aName, nsIAtom *aPrefix, int32_t aNamespaceID,
                   uint16_t aNodeType, nsIAtom *aExtraName,
                   nsNodeInfoManager *aOwnerManager)
{
  if (!sNodeInfoPool) {
    sNodeInfoPool = new nsFixedSizeAllocator();
    if (!sNodeInfoPool)
      return nullptr;

    nsresult rv = sNodeInfoPool->Init("NodeInfo Pool", kNodeInfoPoolSizes,
                                      1, kNodeInfoPoolInitialSize);
    if (NS_FAILED(rv)) {
      delete sNodeInfoPool;
      sNodeInfoPool = nullptr;
      return nullptr;
    }
  }

  
  void* place = sNodeInfoPool->Alloc(sizeof(nsNodeInfo));
  return place ?
    new (place) nsNodeInfo(aName, aPrefix, aNamespaceID, aNodeType, aExtraName,
                           aOwnerManager) :
    nullptr;
}

nsNodeInfo::~nsNodeInfo()
{
  mOwnerManager->RemoveNodeInfo(this);

  NS_RELEASE(mInner.mName);
  NS_IF_RELEASE(mInner.mPrefix);
  NS_IF_RELEASE(mInner.mExtraName);
  NS_RELEASE(mOwnerManager);
}


nsNodeInfo::nsNodeInfo(nsIAtom *aName, nsIAtom *aPrefix, int32_t aNamespaceID,
                       uint16_t aNodeType, nsIAtom* aExtraName,
                       nsNodeInfoManager *aOwnerManager)
{
  CheckValidNodeInfo(aNodeType, aName, aNamespaceID, aExtraName);
  NS_ABORT_IF_FALSE(aOwnerManager, "Invalid aOwnerManager");

  
  NS_ADDREF(mInner.mName = aName);
  NS_IF_ADDREF(mInner.mPrefix = aPrefix);
  mInner.mNamespaceID = aNamespaceID;
  mInner.mNodeType = aNodeType;
  NS_ADDREF(mOwnerManager = aOwnerManager);
  NS_IF_ADDREF(mInner.mExtraName = aExtraName);

  mDocument = aOwnerManager->GetDocument();

  

  
  
  if (aPrefix) {
    mQualifiedName = nsDependentAtomString(mInner.mPrefix) +
                     NS_LITERAL_STRING(":") +
                     nsDependentAtomString(mInner.mName);
  } else {
    mInner.mName->ToString(mQualifiedName);
  }

  MOZ_ASSERT_IF(aNodeType != nsIDOMNode::ELEMENT_NODE &&
                aNodeType != nsIDOMNode::ATTRIBUTE_NODE &&
                aNodeType != UINT16_MAX,
                aNamespaceID == kNameSpaceID_None && !aPrefix);

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
      NS_ABORT_IF_FALSE(aNodeType == UINT16_MAX,
                        "Unknown node type");
  }
}




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
  if (MOZ_UNLIKELY(cb.WantDebugInfo())) {
    char name[72];
    uint32_t nsid = tmp->NamespaceID();
    nsAtomCString localName(tmp->NameAtom());
    if (nsid < ArrayLength(kNSURIs)) {
      PR_snprintf(name, sizeof(name), "nsNodeInfo%s %s", kNSURIs[nsid],
                  localName.get());
    }
    else {
      PR_snprintf(name, sizeof(name), "nsNodeInfo %s", localName.get());
    }

    cb.DescribeRefCountedNode(tmp->mRefCnt.get(), name);
  }
  else {
    NS_IMPL_CYCLE_COLLECTION_DESCRIBE(nsNodeInfo, tmp->mRefCnt.get())
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_RAWPTR(mOwnerManager)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_BEGIN(nsNodeInfo)
  return nsCCUncollectableMarker::sGeneration && tmp->CanSkip();
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_BEGIN(nsNodeInfo)
  return nsCCUncollectableMarker::sGeneration && tmp->CanSkip();
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_BEGIN(nsNodeInfo)
  return nsCCUncollectableMarker::sGeneration && tmp->CanSkip();
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_END


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


bool
nsNodeInfo::NamespaceEquals(const nsAString& aNamespaceURI) const
{
  int32_t nsid =
    nsContentUtils::NameSpaceManager()->GetNameSpaceID(aNamespaceURI);

  return nsINodeInfo::NamespaceEquals(nsid);
}


void
nsNodeInfo::ClearCache()
{
  
  delete sNodeInfoPool;
  sNodeInfoPool = nullptr;
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

bool
nsNodeInfo::CanSkip()
{
  return mDocument &&
    nsCCUncollectableMarker::InGeneration(mDocument->GetMarkedCCGeneration());
}
