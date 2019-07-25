





































#ifndef nsSVGDocument_h__
#define nsSVGDocument_h__

#include "nsXMLDocument.h"
#include "nsIDOMSVGDocument.h"

class nsSVGDocument : public nsXMLDocument,
                      public nsIDOMSVGDocument
{
public:
  using nsDocument::GetElementById;
  using nsDocument::SetDocumentURI;
  nsSVGDocument();
  virtual ~nsSVGDocument();

  NS_DECL_NSIDOMSVGDOCUMENT
  NS_FORWARD_NSIDOMDOCUMENT(nsXMLDocument::)
  NS_FORWARD_NSIDOMNODE(nsXMLDocument::)
  NS_DECL_ISUPPORTS_INHERITED
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsXPCClassInfo* GetClassInfo();
};

#endif
