








#include "nsCOMPtr.h"
#include "mozilla/dom/Element.h" 
#include "mozilla/dom/Comment.h"
#include "mozilla/dom/CommentBinding.h"

using namespace mozilla;
using namespace dom;

namespace mozilla {
namespace dom {

Comment::~Comment()
{
}

NS_IMPL_ISUPPORTS_INHERITED3(Comment, nsGenericDOMDataNode, nsIDOMNode,
                             nsIDOMCharacterData, nsIDOMComment)

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

JSObject*
Comment::WrapNode(JSContext *aCx, JS::Handle<JSObject*> aScope)
{
  return CommentBinding::Wrap(aCx, aScope, this);
}

} 
} 
