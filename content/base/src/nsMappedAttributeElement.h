










#ifndef NS_MAPPEDATTRIBUTEELEMENT_H_
#define NS_MAPPEDATTRIBUTEELEMENT_H_

#include "mozilla/Attributes.h"
#include "nsStyledElement.h"

class nsMappedAttributes;
struct nsRuleData;

typedef void (*nsMapRuleToAttributesFunc)(const nsMappedAttributes* aAttributes, 
                                          nsRuleData* aData);

typedef nsStyledElement nsMappedAttributeElementBase;

class nsMappedAttributeElement : public nsMappedAttributeElementBase
{

protected:

  explicit nsMappedAttributeElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : nsMappedAttributeElementBase(aNodeInfo)
  {}

public:
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;

  static void MapNoAttributesInto(const nsMappedAttributes* aAttributes, 
                                  nsRuleData* aRuleData);

  NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker) MOZ_OVERRIDE;
  virtual bool SetMappedAttribute(nsIDocument* aDocument,
                                    nsIAtom* aName,
                                    nsAttrValue& aValue,
                                    nsresult* aRetval) MOZ_OVERRIDE;
};

#endif 
