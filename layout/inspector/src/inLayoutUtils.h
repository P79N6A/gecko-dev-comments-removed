




































#ifndef __inLayoutUtils_h__
#define __inLayoutUtils_h__

#include "nsCOMPtr.h"
#include "nsIDOMElement.h"
#include "nsIDOMWindowInternal.h"
#include "nsIPresShell.h"
#include "nsIFrame.h"
#include "nsIRenderingContext.h"
#include "nsIEventStateManager.h"
#include "nsIDOMDocument.h"
#include "nsBindingManager.h"

class inLayoutUtils
{
public:
  static nsIDOMWindowInternal* GetWindowFor(nsIDOMNode* aNode);
  static nsIDOMWindowInternal* GetWindowFor(nsIDOMDocument* aDoc);
  static nsIPresShell* GetPresShellFor(nsISupports* aThing);
  static nsIFrame* GetFrameFor(nsIDOMElement* aElement, nsIPresShell* aShell);
  static nsIEventStateManager* GetEventStateManagerFor(nsIDOMElement *aElement);
  static nsBindingManager* GetBindingManagerFor(nsIDOMNode* aNode);
  static nsIDOMDocument* GetSubDocumentFor(nsIDOMNode* aNode);
  static nsIDOMNode* GetContainerFor(nsIDOMDocument* aDoc);
};

#endif 
