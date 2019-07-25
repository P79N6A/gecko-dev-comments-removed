




































#ifndef __inLayoutUtils_h__
#define __inLayoutUtils_h__

class nsBindingManager;
class nsIDOMDocument;
class nsIDOMElement;
class nsIDOMNode;
class nsIDOMWindowInternal;
class nsEventStateManager;
class nsIFrame;
class nsIPresShell;
class nsISupports;

class inLayoutUtils
{
public:
  static nsIDOMWindowInternal* GetWindowFor(nsIDOMNode* aNode);
  static nsIDOMWindowInternal* GetWindowFor(nsIDOMDocument* aDoc);
  static nsIPresShell* GetPresShellFor(nsISupports* aThing);
  static nsIFrame* GetFrameFor(nsIDOMElement* aElement);
  static nsEventStateManager* GetEventStateManagerFor(nsIDOMElement *aElement);
  static nsBindingManager* GetBindingManagerFor(nsIDOMNode* aNode);
  static nsIDOMDocument* GetSubDocumentFor(nsIDOMNode* aNode);
  static nsIDOMNode* GetContainerFor(nsIDOMDocument* aDoc);
};

#endif 
