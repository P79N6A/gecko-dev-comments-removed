




#ifndef __inLayoutUtils_h__
#define __inLayoutUtils_h__

class nsIDocument;
class nsIDOMDocument;
class nsIDOMElement;
class nsIDOMNode;
class nsIDOMWindow;
class nsIFrame;
class nsIPresShell;
class nsISupports;

namespace mozilla {
class EventStateManager;
}

class inLayoutUtils
{
public:
  static nsIDOMWindow* GetWindowFor(nsIDOMNode* aNode);
  static nsIDOMWindow* GetWindowFor(nsIDOMDocument* aDoc);
  static nsIPresShell* GetPresShellFor(nsISupports* aThing);
  static nsIFrame* GetFrameFor(nsIDOMElement* aElement);
  static mozilla::EventStateManager*
           GetEventStateManagerFor(nsIDOMElement *aElement);
  static nsIDOMDocument* GetSubDocumentFor(nsIDOMNode* aNode);
  static nsIDOMNode* GetContainerFor(const nsIDocument& aDoc);
};

#endif 
