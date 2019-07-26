








#include "nsCOMPtr.h"
#include "mozilla/dom/Element.h" 
#include "Comment.h"

using namespace mozilla;
using namespace dom;


DOMCI_NODE_DATA(Comment, Comment)

nsresult
NS_NewCommentNode(nsIContent** aInstancePtrResult,
                  nsNodeInfoManager *aNodeInfoManager)
{
  NS_PRECONDITION(aNodeInfoManager, "Missing nodeinfo manager");

  *aInstancePtrResult = nullptr;

  nsCOMPtr<nsINodeInfo> ni = aNodeInfoManager->GetCommentNodeInfo();
  NS_ENSURE_TRUE(ni, NS_ERROR_OUT_OF_MEMORY);

  Comment *instance = new Comment(ni.forget());
  if (!instance) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aInstancePtrResult = instance);

  return NS_OK;
}

namespace mozilla {
namespace dom {

Comment::Comment(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericDOMDataNode(aNodeInfo)
{
  NS_ABORT_IF_FALSE(mNodeInfo->NodeType() == nsIDOMNode::COMMENT_NODE,
                    "Bad NodeType in aNodeInfo");
}

Comment::~Comment()
{
}


NS_INTERFACE_TABLE_HEAD(Comment)
  NS_NODE_INTERFACE_TABLE3(Comment, nsIDOMNode, nsIDOMCharacterData,
                           nsIDOMComment)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Comment)
NS_INTERFACE_MAP_END_INHERITING(nsGenericDOMDataNode)


NS_IMPL_ADDREF_INHERITED(Comment, nsGenericDOMDataNode)
NS_IMPL_RELEASE_INHERITED(Comment, nsGenericDOMDataNode)


bool
Comment::IsNodeOfType(uint32_t aFlags) const
{
  return !(aFlags & ~(eCONTENT | eCOMMENT | eDATA_NODE));
}

nsGenericDOMDataNode*
Comment::CloneDataNode(nsINodeInfo *aNodeInfo, bool aCloneText) const
{
  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
  Comment *it = new Comment(ni.forget());
  if (it && aCloneText) {
    it->mText = mText;
  }

  return it;
}

#ifdef DEBUG
void
Comment::List(FILE* out, int32_t aIndent) const
{
  int32_t indx;
  for (indx = aIndent; --indx >= 0; ) fputs("  ", out);

  fprintf(out, "Comment@%p refcount=%d<!--", (void*)this, mRefCnt.get());

  nsAutoString tmp;
  ToCString(tmp, 0, mText.GetLength());
  fputs(NS_LossyConvertUTF16toASCII(tmp).get(), out);

  fputs("-->\n", out);
}
#endif

} 
} 
