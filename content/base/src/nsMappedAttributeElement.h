











































#ifndef NS_MAPPEDATTRIBUTEELEMENT_H_
#define NS_MAPPEDATTRIBUTEELEMENT_H_

#include "nsStyledElement.h"
#include "nsDOMMemoryReporter.h"

class nsMappedAttributes;
struct nsRuleData;

typedef void (*nsMapRuleToAttributesFunc)(const nsMappedAttributes* aAttributes, 
                                          nsRuleData* aData);

typedef nsStyledElement nsMappedAttributeElementBase;

class nsMappedAttributeElement : public nsMappedAttributeElementBase
{

protected:

  nsMappedAttributeElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsMappedAttributeElementBase(aNodeInfo)
  {}

public:
  NS_DECL_AND_IMPL_DOM_MEMORY_REPORTER_SIZEOF(nsMappedAttributeElement,
                                              nsMappedAttributeElementBase)

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);

  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;

  static void MapNoAttributesInto(const nsMappedAttributes* aAttributes, 
                                  nsRuleData* aRuleData);

  NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker);
  virtual bool SetMappedAttribute(nsIDocument* aDocument,
                                    nsIAtom* aName,
                                    nsAttrValue& aValue,
                                    nsresult* aRetval);
};

#endif 
