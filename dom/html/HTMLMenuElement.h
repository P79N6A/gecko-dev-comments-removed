




#ifndef mozilla_dom_HTMLMenuElement_h
#define mozilla_dom_HTMLMenuElement_h

#include "mozilla/Attributes.h"
#include "nsIDOMHTMLMenuElement.h"
#include "nsIHTMLMenu.h"
#include "nsGenericHTMLElement.h"

namespace mozilla {
namespace dom {

class HTMLMenuElement MOZ_FINAL : public nsGenericHTMLElement,
                                  public nsIDOMHTMLMenuElement,
                                  public nsIHTMLMenu
{
public:
  explicit HTMLMenuElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLMenuElement, menu)

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMHTMLMENUELEMENT

  
  NS_DECL_NSIHTMLMENU

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult) MOZ_OVERRIDE;

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  uint8_t GetType() const { return mType; }

  

  
  void SetType(const nsAString& aType, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::type, aType, aError);
  }

  
  void SetLabel(const nsAString& aLabel, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::label, aLabel, aError);
  }

  bool Compact() const
  {
    return GetBoolAttr(nsGkAtoms::compact);
  }
  void SetCompact(bool aCompact, ErrorResult& aError)
  {
    SetHTMLBoolAttr(nsGkAtoms::compact, aCompact, aError);
  }

  

  already_AddRefed<nsIMenuBuilder> CreateBuilder();

  

protected:
  virtual ~HTMLMenuElement();

  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;


protected:
  static bool CanLoadIcon(nsIContent* aContent, const nsAString& aIcon);

  void BuildSubmenu(const nsAString& aLabel,
                    nsIContent* aContent,
                    nsIMenuBuilder* aBuilder);

  void TraverseContent(nsIContent* aContent,
                       nsIMenuBuilder* aBuilder,
                       int8_t& aSeparator);

  void AddSeparator(nsIMenuBuilder* aBuilder, int8_t& aSeparator);

  uint8_t mType;
};

} 
} 

#endif 
