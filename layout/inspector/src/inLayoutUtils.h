




#ifndef __inLayoutUtils_h__
#define __inLayoutUtils_h__

class nsIDOMDocument;
class nsIDOMElement;
class nsIDOMNode;
class nsIDOMWindow;
class nsEventStateManager;
class nsIFrame;
class nsIPresShell;
class nsISupports;

class inLayoutUtils
{
public:
  static nsIDOMWindow* GetWindowFor(nsIDOMNode* aNode);
  static nsIDOMWindow* GetWindowFor(nsIDOMDocument* aDoc);
  static nsIPresShell* GetPresShellFor(nsISupports* aThing);
  static nsIFrame* GetFrameFor(nsIDOMElement* aElement);
  static nsEventStateManager* GetEventStateManagerFor(nsIDOMElement *aElement);
  static nsIDOMDocument* GetSubDocumentFor(nsIDOMNode* aNode);
  static nsIDOMNode* GetContainerFor(nsIDOMDocument* aDoc);
};

#endif 
