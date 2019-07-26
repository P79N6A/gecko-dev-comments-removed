




#ifndef mozilla_dom_Text_h
#define mozilla_dom_Text_h

#include "nsGenericDOMDataNode.h"
#include "mozilla/ErrorResult.h"

namespace mozilla {
namespace dom {

class Text : public nsGenericDOMDataNode
{
public:
  Text(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericDOMDataNode(aNodeInfo)
  {}

  using nsGenericDOMDataNode::GetWholeText;

  
  already_AddRefed<Text> SplitText(uint32_t aOffset, ErrorResult& rv);
  void GetWholeText(nsAString& aWholeText, ErrorResult& rv)
  {
    rv = GetWholeText(aWholeText);
  }
};

} 
} 

#endif 
