




#ifndef mozilla_dom_SVGDocument_h
#define mozilla_dom_SVGDocument_h

#include "mozilla/dom/XMLDocument.h"
#include "nsIDOMSVGDocument.h"

namespace mozilla {
namespace dom {

class SVGDocument : public XMLDocument,
                    public nsIDOMSVGDocument
{
public:
  using nsDocument::GetElementById;
  using nsDocument::SetDocumentURI;
  SVGDocument();
  virtual ~SVGDocument();

  NS_DECL_NSIDOMSVGDOCUMENT
  NS_FORWARD_NSIDOMDOCUMENT(mozilla::dom::XMLDocument::)
  
  using nsDocument::GetImplementation;
  using nsDocument::GetTitle;
  using nsDocument::SetTitle;
  using nsDocument::GetLastStyleSheetSet;
  using nsDocument::MozSetImageElement;
  using nsDocument::GetMozFullScreenElement;

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_DECL_ISUPPORTS_INHERITED
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsXPCClassInfo* GetClassInfo();
};

} 
} 

#endif 
