



#ifndef HTMLFontElement_h___
#define HTMLFontElement_h___

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"

namespace mozilla {
namespace dom {

class HTMLFontElement MOZ_FINAL : public nsGenericHTMLElement
{
public:
  explicit HTMLFontElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
  }

  void GetColor(nsString& aColor)
  {
    GetHTMLAttr(nsGkAtoms::color, aColor);
  }
  void SetColor(const nsAString& aColor, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::color, aColor, aError);
  }
  void GetFace(nsString& aFace)
  {
    GetHTMLAttr(nsGkAtoms::face, aFace);
  }
  void SetFace(const nsAString& aFace, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::face, aFace, aError);
  }
  void GetSize(nsString& aSize)
  {
    GetHTMLAttr(nsGkAtoms::size, aSize);
  }
  void SetSize(const nsAString& aSize, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::size, aSize, aError);
  }

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult) MOZ_OVERRIDE;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const MOZ_OVERRIDE;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const MOZ_OVERRIDE;
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

protected:
  virtual ~HTMLFontElement();

  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;

private:
  static void MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                                    nsRuleData* aData);
};

} 
} 

#endif 
