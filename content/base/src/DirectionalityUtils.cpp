





#include "mozilla/dom/DirectionalityUtils.h"
#include "nsINode.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "mozilla/dom/Element.h"
#include "nsIDOMNodeFilter.h"
#include "nsTreeWalker.h"
#include "nsIDOMHTMLDocument.h"


namespace mozilla {

namespace directionality {

typedef mozilla::dom::Element Element;

Directionality
RecomputeDirectionality(Element* aElement, bool aNotify)
{
  Directionality dir = eDir_LTR;

  if (aElement->HasValidDir()) {
    dir = aElement->GetDirectionality();
  } else {
    Element* parent = aElement->GetElementParent();
    if (parent) {
      
      
      
      Directionality parentDir = parent->GetDirectionality();
      if (parentDir != eDir_NotSet) {
        dir = parentDir;
      }
    } else {
      
      
      Directionality documentDir =
        aElement->OwnerDoc()->GetDocumentDirectionality();
      if (documentDir != eDir_NotSet) {
        dir = documentDir;
      }
    }
    
    aElement->SetDirectionality(dir, aNotify);
  }
  return dir;
}

void
SetDirectionalityOnDescendants(Element* aElement, Directionality aDir,
                               bool aNotify)
{
  for (nsIContent* child = aElement->GetFirstChild(); child; ) {
    if (!child->IsElement()) {
      child = child->GetNextNode(aElement);
      continue;
    }

    Element* element = child->AsElement();
    if (element->HasValidDir()) {
      child = child->GetNextNonChildNode(aElement);
      continue;
    }
    element->SetDirectionality(aDir, aNotify);
    child = child->GetNextNode(aElement);
  }
}

} 

} 

