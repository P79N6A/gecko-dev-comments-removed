




#ifndef __inLayoutUtils_h__
#define __inLayoutUtils_h__

class nsIDocument;
class nsIDOMDocument;
class nsIDOMElement;
class nsIDOMNode;

namespace mozilla {
class EventStateManager;
}

class inLayoutUtils
{
public:
  static mozilla::EventStateManager*
           GetEventStateManagerFor(nsIDOMElement *aElement);
  static nsIDOMDocument* GetSubDocumentFor(nsIDOMNode* aNode);
  static nsIDOMNode* GetContainerFor(const nsIDocument& aDoc);
};

#endif 
