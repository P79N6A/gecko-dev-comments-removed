




#ifndef nsIDocumentInlines_h
#define nsIDocumentInlines_h

#include "nsIDocument.h"
#include "mozilla/dom/HTMLBodyElement.h"

inline mozilla::dom::HTMLBodyElement*
nsIDocument::GetBodyElement()
{
  return static_cast<mozilla::dom::HTMLBodyElement*>(GetHtmlChildElement(nsGkAtoms::body));
}

#endif 
