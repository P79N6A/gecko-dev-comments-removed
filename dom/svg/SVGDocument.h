




#ifndef mozilla_dom_SVGDocument_h
#define mozilla_dom_SVGDocument_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/XMLDocument.h"

class nsSVGElement;

namespace mozilla {
namespace dom {

class SVGForeignObjectElement;

class SVGDocument MOZ_FINAL : public XMLDocument
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
                                 bool aNotify) MOZ_OVERRIDE;
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  void GetDomain(nsAString& aDomain, ErrorResult& aRv);
  nsSVGElement* GetRootElement(ErrorResult& aRv);

  virtual SVGDocument* AsSVGDocument() MOZ_OVERRIDE {
    return this;
  }

private:
  void EnsureNonSVGUserAgentStyleSheetsLoaded();

  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;

  bool mHasLoadedNonSVGUserAgentStyleSheets;
};

} 
} 

#endif 
