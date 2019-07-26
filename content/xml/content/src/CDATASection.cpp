




#include "mozilla/dom/CDATASection.h"
#include "mozilla/dom/CDATASectionBinding.h"
#include "mozilla/IntegerPrintfMacros.h"

namespace mozilla {
namespace dom {

CDATASection::~CDATASection()
{
}

NS_IMPL_ISUPPORTS_INHERITED4(CDATASection, nsGenericDOMDataNode, nsIDOMNode,
                             nsIDOMCharacterData, nsIDOMText,
                             nsIDOMCDATASection)

JSObject*
CDATASection::WrapNode(JSContext *aCx)
{
  return CDATASectionBinding::Wrap(aCx, this);
}

bool
CDATASection::IsNodeOfType(uint32_t aFlags) const
{
  return !(aFlags & ~(eCONTENT | eTEXT | eDATA_NODE));
}

nsGenericDOMDataNode*
CDATASection::CloneDataNode(nsINodeInfo *aNodeInfo, bool aCloneText) const
{
  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
  CDATASection *it = new CDATASection(ni.forget());
  if (it && aCloneText) {
    it->mText = mText;
  }

  return it;
}

#ifdef DEBUG
void
CDATASection::List(FILE* out, int32_t aIndent) const
{
  int32_t index;
  for (index = aIndent; --index >= 0; ) fputs("  ", out);

  fprintf(out, "CDATASection refcount=%" PRIuPTR "<", mRefCnt.get());

  nsAutoString tmp;
  ToCString(tmp, 0, mText.GetLength());
  fputs(NS_LossyConvertUTF16toASCII(tmp).get(), out);

  fputs(">\n", out);
}

void
CDATASection::DumpContent(FILE* out, int32_t aIndent,
                               bool aDumpAll) const {
}
#endif

} 
} 
