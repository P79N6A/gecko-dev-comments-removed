





#ifndef mozilla_dom_SVGDocument_h
#define mozilla_dom_SVGDocument_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/XMLDocument.h"

class nsSVGElement;

namespace mozilla {
namespace dom {

class SVGForeignObjectElement;

class SVGDocument final : public XMLDocument
{
  friend class SVGForeignObjectElement; 

public:
  SVGDocument()
    : XMLDocument("image/svg+xml")
    , mHasLoadedNonSVGUserAgentStyleSheets(false)
  {
    mType = eSVG;
  }

  virtual nsresult InsertChildAt(nsIContent* aKid, uint32_t aIndex,
                                 bool aNotify) override;
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

  
  void GetDomain(nsAString& aDomain, ErrorResult& aRv);
  nsSVGElement* GetRootElement(ErrorResult& aRv);

  virtual SVGDocument* AsSVGDocument() override {
    return this;
  }

private:
  void EnsureNonSVGUserAgentStyleSheetsLoaded();

  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

  bool mHasLoadedNonSVGUserAgentStyleSheets;
};

} 
} 

#endif 
