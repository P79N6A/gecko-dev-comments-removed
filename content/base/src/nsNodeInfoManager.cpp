








































#include "nsNodeInfoManager.h"
#include "nsNodeInfo.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIAtom.h"
#include "nsIDocument.h"
#include "nsIPrincipal.h"
#include "nsIURI.h"
#include "nsContentUtils.h"
#include "nsReadableUtils.h"
#include "nsGkAtoms.h"
#include "nsComponentManagerUtils.h"
#include "nsLayoutStatics.h"

PLHashNumber
nsNodeInfoManager::GetNodeInfoInnerHashValue(const void *key)
{
  NS_ASSERTION(key, "Null key passed to nsNodeInfo::GetHashValue!");

  const nsINodeInfo::nsNodeInfoInner *node =
    reinterpret_cast<const nsINodeInfo::nsNodeInfoInner *>(key);

  
  return (PLHashNumber(NS_PTR_TO_INT32(node->mName)) & 0xffff) >> 8;
}


PRIntn
nsNodeInfoManager::NodeInfoInnerKeyCompare(const void *key1, const void *key2)
{
  NS_ASSERTION(key1 && key2, "Null key passed to NodeInfoInnerKeyCompare!");

  const nsINodeInfo::nsNodeInfoInner *node1 =
    reinterpret_cast<const nsINodeInfo::nsNodeInfoInner *>(key1);
  const nsINodeInfo::nsNodeInfoInner *node2 =
    reinterpret_cast<const nsINodeInfo::nsNodeInfoInner *>(key2);

  return (node1->mName == node2->mName &&
          node1->mPrefix == node2->mPrefix &&
          node1->mNamespaceID == node2->mNamespaceID);
}


nsNodeInfoManager::nsNodeInfoManager()
  : mDocument(nsnull),
    mPrincipal(nsnull),
    mTextNodeInfo(nsnull),
    mCommentNodeInfo(nsnull),
    mDocumentNodeInfo(nsnull)
{
  nsLayoutStatics::AddRef();

  mNodeInfoHash = PL_NewHashTable(32, GetNodeInfoInnerHashValue,
                                  NodeInfoInnerKeyCompare,
                                  PL_CompareValues, nsnull, nsnull);
}


nsNodeInfoManager::~nsNodeInfoManager()
{
  if (mNodeInfoHash)
    PL_HashTableDestroy(mNodeInfoHash);

  
  NS_IF_RELEASE(mPrincipal);

  nsLayoutStatics::Release();
}


nsrefcnt
nsNodeInfoManager::AddRef()
{
  NS_PRECONDITION(PRInt32(mRefCnt) >= 0, "illegal refcnt");

  nsrefcnt count = PR_AtomicIncrement((PRInt32*)&mRefCnt);
  NS_LOG_ADDREF(this, count, "nsNodeInfoManager", sizeof(*this));

  return count;
}

nsrefcnt
nsNodeInfoManager::Release()
{
  NS_PRECONDITION(0 != mRefCnt, "dup release");

  nsrefcnt count = PR_AtomicDecrement((PRInt32 *)&mRefCnt);
  NS_LOG_RELEASE(this, count, "nsNodeInfoManager");
  if (count == 0) {
    mRefCnt = 1; 
    delete this;
  }

  return count;
}

nsresult
nsNodeInfoManager::Init(nsIDocument *aDocument)
{
  NS_ENSURE_TRUE(mNodeInfoHash, NS_ERROR_OUT_OF_MEMORY);

  NS_PRECONDITION(!mPrincipal,
                  "Being inited when we already have a principal?");
  nsresult rv = CallCreateInstance("@mozilla.org/nullprincipal;1",
                                   &mPrincipal);
  NS_ENSURE_TRUE(mPrincipal, rv);

  mDefaultPrincipal = mPrincipal;

  mDocument = aDocument;

  return NS_OK;
}

void
nsNodeInfoManager::DropDocumentReference()
{
  mDocument = nsnull;
}


nsresult
nsNodeInfoManager::GetNodeInfo(nsIAtom *aName, nsIAtom *aPrefix,
                               PRInt32 aNamespaceID, nsINodeInfo** aNodeInfo)
{
  NS_ENSURE_ARG_POINTER(aName);
  NS_ASSERTION(!aName->Equals(EmptyString()),
               "Don't pass an empty string to GetNodeInfo, fix caller.");

  nsINodeInfo::nsNodeInfoInner tmpKey(aName, aPrefix, aNamespaceID);

  void *node = PL_HashTableLookup(mNodeInfoHash, &tmpKey);

  if (node) {
    *aNodeInfo = static_cast<nsINodeInfo *>(node);

    NS_ADDREF(*aNodeInfo);

    return NS_OK;
  }

  nsNodeInfo *newNodeInfo = nsNodeInfo::Create();
  NS_ENSURE_TRUE(newNodeInfo, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(newNodeInfo);

  nsresult rv = newNodeInfo->Init(aName, aPrefix, aNamespaceID, this);
  NS_ENSURE_SUCCESS(rv, rv);

  PLHashEntry *he;
  he = PL_HashTableAdd(mNodeInfoHash, &newNodeInfo->mInner, newNodeInfo);
  NS_ENSURE_TRUE(he, NS_ERROR_OUT_OF_MEMORY);

  *aNodeInfo = newNodeInfo;

  return NS_OK;
}


nsresult
nsNodeInfoManager::GetNodeInfo(const nsAString& aName, nsIAtom *aPrefix,
                               PRInt32 aNamespaceID, nsINodeInfo** aNodeInfo)
{
  nsCOMPtr<nsIAtom> name = do_GetAtom(aName);
  return nsNodeInfoManager::GetNodeInfo(name, aPrefix, aNamespaceID,
                                        aNodeInfo);
}


nsresult
nsNodeInfoManager::GetNodeInfo(const nsAString& aQualifiedName,
                               const nsAString& aNamespaceURI,
                               nsINodeInfo** aNodeInfo)
{
  NS_ENSURE_ARG(!aQualifiedName.IsEmpty());

  nsAString::const_iterator start, end;
  aQualifiedName.BeginReading(start);
  aQualifiedName.EndReading(end);

  nsCOMPtr<nsIAtom> prefixAtom;

  nsAString::const_iterator iter(start);

  if (FindCharInReadable(':', iter, end)) {
    prefixAtom = do_GetAtom(Substring(start, iter));
    NS_ENSURE_TRUE(prefixAtom, NS_ERROR_OUT_OF_MEMORY);

    start = ++iter; 

    if (iter == end) {
      

      return NS_ERROR_INVALID_ARG;
    }
  }

  nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(Substring(start, end));
  NS_ENSURE_TRUE(nameAtom, NS_ERROR_OUT_OF_MEMORY);

  PRInt32 nsid = kNameSpaceID_None;

  if (!aNamespaceURI.IsEmpty()) {
    nsresult rv = nsContentUtils::NameSpaceManager()->
      RegisterNameSpace(aNamespaceURI, nsid);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return GetNodeInfo(nameAtom, prefixAtom, nsid, aNodeInfo);
}

already_AddRefed<nsINodeInfo>
nsNodeInfoManager::GetTextNodeInfo()
{
  if (!mTextNodeInfo) {
    GetNodeInfo(nsGkAtoms::textTagName, nsnull, kNameSpaceID_None,
                &mTextNodeInfo);
  }
  else {
    NS_ADDREF(mTextNodeInfo);
  }

  return mTextNodeInfo;
}

already_AddRefed<nsINodeInfo>
nsNodeInfoManager::GetCommentNodeInfo()
{
  if (!mCommentNodeInfo) {
    GetNodeInfo(nsGkAtoms::commentTagName, nsnull, kNameSpaceID_None,
                &mCommentNodeInfo);
  }
  else {
    NS_ADDREF(mCommentNodeInfo);
  }

  return mCommentNodeInfo;
}

already_AddRefed<nsINodeInfo>
nsNodeInfoManager::GetDocumentNodeInfo()
{
  if (!mDocumentNodeInfo) {
    GetNodeInfo(nsGkAtoms::documentNodeName, nsnull, kNameSpaceID_None,
                &mDocumentNodeInfo);
  }
  else {
    NS_ADDREF(mDocumentNodeInfo);
  }

  return mDocumentNodeInfo;
}

void
nsNodeInfoManager::SetDocumentPrincipal(nsIPrincipal *aPrincipal)
{
  NS_RELEASE(mPrincipal);
  if (!aPrincipal) {
    aPrincipal = mDefaultPrincipal;
  }

  NS_ASSERTION(aPrincipal, "Must have principal by this point!");
  
  NS_ADDREF(mPrincipal = aPrincipal);
}

void
nsNodeInfoManager::RemoveNodeInfo(nsNodeInfo *aNodeInfo)
{
  NS_PRECONDITION(aNodeInfo, "Trying to remove null nodeinfo from manager!");

  
  if (aNodeInfo == mTextNodeInfo) {
    mTextNodeInfo = nsnull;
  }
  else if (aNodeInfo == mCommentNodeInfo) {
    mCommentNodeInfo = nsnull;
  }
  else if (aNodeInfo == mDocumentNodeInfo) {
    mDocumentNodeInfo = nsnull;
  }

#ifdef DEBUG
  PRBool ret =
#endif
  PL_HashTableRemove(mNodeInfoHash, &aNodeInfo->mInner);

  NS_POSTCONDITION(ret, "Can't find nsINodeInfo to remove!!!");
}
