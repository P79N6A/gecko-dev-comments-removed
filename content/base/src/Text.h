




#ifndef mozilla_dom_Text_h
#define mozilla_dom_Text_h

#include "nsGenericDOMDataNode.h"
#include "mozilla/ErrorResult.h"

namespace mozilla {
namespace dom {

class Text : public nsGenericDOMDataNode
{
public:
  Text(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : nsGenericDOMDataNode(aNodeInfo)
  {}

  Text(already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo)
    : nsGenericDOMDataNode(aNodeInfo)
  {}

  using nsGenericDOMDataNode::GetWholeText;

  
  already_AddRefed<Text> SplitText(uint32_t aOffset, ErrorResult& rv);
  void GetWholeText(nsAString& aWholeText, ErrorResult& rv)
  {
    rv = GetWholeText(aWholeText);
  }

  static already_AddRefed<Text>
  Constructor(const GlobalObject& aGlobal,
              const nsAString& aData, ErrorResult& aRv);
};

} 
} 

inline mozilla::dom::Text* nsINode::GetAsText()
{
  return IsNodeOfType(eTEXT) ? static_cast<mozilla::dom::Text*>(this)
                             : nullptr;
}

inline const mozilla::dom::Text* nsINode::GetAsText() const
{
  return IsNodeOfType(eTEXT) ? static_cast<const mozilla::dom::Text*>(this)
                             : nullptr;
}

#endif 
