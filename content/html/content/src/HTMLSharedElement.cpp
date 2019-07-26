




#include "mozilla/dom/HTMLSharedElement.h"

#include "nsAttrValueInlines.h"
#include "nsStyleConsts.h"
#include "nsRuleData.h"
#include "nsMappedAttributes.h"
#include "nsContentUtils.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Shared)
DOMCI_DATA(HTMLParamElement, mozilla::dom::HTMLSharedElement)
DOMCI_DATA(HTMLBaseElement, mozilla::dom::HTMLSharedElement)
DOMCI_DATA(HTMLDirectoryElement, mozilla::dom::HTMLSharedElement)
DOMCI_DATA(HTMLQuoteElement, mozilla::dom::HTMLSharedElement)
DOMCI_DATA(HTMLHeadElement, mozilla::dom::HTMLSharedElement)
DOMCI_DATA(HTMLHtmlElement, mozilla::dom::HTMLSharedElement)

namespace mozilla {
namespace dom {

extern nsAttrValue::EnumTable kListTypeTable[];

HTMLSharedElement::~HTMLSharedElement()
{
}

NS_IMPL_ADDREF_INHERITED(HTMLSharedElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLSharedElement, Element)

nsIClassInfo*
HTMLSharedElement::GetClassInfoInternal()
{
  if (mNodeInfo->Equals(nsGkAtoms::param)) {
    return NS_GetDOMClassInfoInstance(eDOMClassInfo_HTMLParamElement_id);
  }
  if (mNodeInfo->Equals(nsGkAtoms::base)) {
    return NS_GetDOMClassInfoInstance(eDOMClassInfo_HTMLBaseElement_id);
  }
  if (mNodeInfo->Equals(nsGkAtoms::dir)) {
    return NS_GetDOMClassInfoInstance(eDOMClassInfo_HTMLDirectoryElement_id);
  }
  if (mNodeInfo->Equals(nsGkAtoms::q)) {
    return NS_GetDOMClassInfoInstance(eDOMClassInfo_HTMLQuoteElement_id);
  }
  if (mNodeInfo->Equals(nsGkAtoms::blockquote)) {
    return NS_GetDOMClassInfoInstance(eDOMClassInfo_HTMLQuoteElement_id);
  }
  if (mNodeInfo->Equals(nsGkAtoms::head)) {
    return NS_GetDOMClassInfoInstance(eDOMClassInfo_HTMLHeadElement_id);
  }
  if (mNodeInfo->Equals(nsGkAtoms::html)) {
    return NS_GetDOMClassInfoInstance(eDOMClassInfo_HTMLHtmlElement_id);
  }
  return nullptr;
}


NS_INTERFACE_TABLE_HEAD(HTMLSharedElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_AMBIGUOUS_BEGIN(HTMLSharedElement,
                                                  nsIDOMHTMLParamElement)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE_AMBIGUOUS(HTMLSharedElement,
                                                         nsGenericHTMLElement,
                                                         nsIDOMHTMLParamElement)
  NS_INTERFACE_MAP_ENTRY_IF_TAG(nsIDOMHTMLParamElement, param)
  NS_INTERFACE_MAP_ENTRY_IF_TAG(nsIDOMHTMLBaseElement, base)
  NS_INTERFACE_MAP_ENTRY_IF_TAG(nsIDOMHTMLDirectoryElement, dir)
  NS_INTERFACE_MAP_ENTRY_IF_TAG(nsIDOMHTMLQuoteElement, q)
  NS_INTERFACE_MAP_ENTRY_IF_TAG(nsIDOMHTMLQuoteElement, blockquote)
  NS_INTERFACE_MAP_ENTRY_IF_TAG(nsIDOMHTMLHeadElement, head)
  NS_INTERFACE_MAP_ENTRY_IF_TAG(nsIDOMHTMLHtmlElement, html)

  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO_GETTER(GetClassInfoInternal)
NS_HTML_CONTENT_INTERFACE_MAP_END


NS_IMPL_ELEMENT_CLONE(HTMLSharedElement)


NS_IMPL_STRING_ATTR(HTMLSharedElement, Name, name)
NS_IMPL_STRING_ATTR(HTMLSharedElement, Type, type)
NS_IMPL_STRING_ATTR(HTMLSharedElement, Value, value)
NS_IMPL_STRING_ATTR(HTMLSharedElement, ValueType, valuetype)


NS_IMPL_BOOL_ATTR(HTMLSharedElement, Compact, compact)


NS_IMPL_URI_ATTR(HTMLSharedElement, Cite, cite)





NS_IMPL_STRING_ATTR(HTMLSharedElement, Version, version)


NS_IMPL_STRING_ATTR(HTMLSharedElement, Target, target)

NS_IMETHODIMP
HTMLSharedElement::GetHref(nsAString& aValue)
{
  nsAutoString href;
  GetAttr(kNameSpaceID_None, nsGkAtoms::href, href);

  nsCOMPtr<nsIURI> uri;
  nsIDocument* doc = OwnerDoc();
  nsContentUtils::NewURIWithDocumentCharset(
    getter_AddRefs(uri), href, doc, doc->GetDocumentURI());

  if (!uri) {
    aValue = href;
    return NS_OK;
  }
  
  nsAutoCString spec;
  uri->GetSpec(spec);
  CopyUTF8toUTF16(spec, aValue);

  return NS_OK;
}

NS_IMETHODIMP
HTMLSharedElement::SetHref(const nsAString& aValue)
{
  return SetAttrHelper(nsGkAtoms::href, aValue);
}


bool
HTMLSharedElement::ParseAttribute(int32_t aNamespaceID,
                                  nsIAtom* aAttribute,
                                  const nsAString& aValue,
                                  nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None &&
      mNodeInfo->Equals(nsGkAtoms::dir)) {
    if (aAttribute == nsGkAtoms::type) {
      return aResult.ParseEnumValue(aValue, mozilla::dom::kListTypeTable, false);
    }
    if (aAttribute == nsGkAtoms::start) {
      return aResult.ParseIntWithBounds(aValue, 1);
    }
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

static void
DirectoryMapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                               nsRuleData* aData)
{
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(List)) {
    nsCSSValue* listStyleType = aData->ValueForListStyleType();
    if (listStyleType->GetUnit() == eCSSUnit_Null) {
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::type);
      if (value) {
        if (value->Type() == nsAttrValue::eEnum) {
          listStyleType->SetIntValue(value->GetEnumValue(), eCSSUnit_Enumerated);
        } else {
          listStyleType->SetIntValue(NS_STYLE_LIST_STYLE_DISC, eCSSUnit_Enumerated);
        }
      }
    }
  }

  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(bool)
HTMLSharedElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  if (mNodeInfo->Equals(nsGkAtoms::dir)) {
    static const MappedAttributeEntry attributes[] = {
      { &nsGkAtoms::type },
      
      { nullptr} 
    };
  
    static const MappedAttributeEntry* const map[] = {
      attributes,
      sCommonAttributeMap,
    };

    return FindAttributeDependence(aAttribute, map);
  }

  return nsGenericHTMLElement::IsAttributeMapped(aAttribute);
}

static void
SetBaseURIUsingFirstBaseWithHref(nsIDocument* aDocument, nsIContent* aMustMatch)
{
  NS_PRECONDITION(aDocument, "Need a document!");

  for (nsIContent* child = aDocument->GetFirstChild(); child;
       child = child->GetNextNode()) {
    if (child->IsHTML(nsGkAtoms::base) &&
        child->HasAttr(kNameSpaceID_None, nsGkAtoms::href)) {
      if (aMustMatch && child != aMustMatch) {
        return;
      }

      
      nsAutoString href;
      child->GetAttr(kNameSpaceID_None, nsGkAtoms::href, href);

      nsCOMPtr<nsIURI> newBaseURI;
      nsContentUtils::NewURIWithDocumentCharset(
        getter_AddRefs(newBaseURI), href, aDocument,
        aDocument->GetDocumentURI());

      
      nsresult rv = aDocument->SetBaseURI(newBaseURI);
      if (NS_FAILED(rv)) {
        aDocument->SetBaseURI(nullptr);
      }
      return;
    }
  }

  aDocument->SetBaseURI(nullptr);
}

static void
SetBaseTargetUsingFirstBaseWithTarget(nsIDocument* aDocument,
                                      nsIContent* aMustMatch)
{
  NS_PRECONDITION(aDocument, "Need a document!");

  for (nsIContent* child = aDocument->GetFirstChild(); child;
       child = child->GetNextNode()) {
    if (child->IsHTML(nsGkAtoms::base) &&
        child->HasAttr(kNameSpaceID_None, nsGkAtoms::target)) {
      if (aMustMatch && child != aMustMatch) {
        return;
      }

      nsString target;
      child->GetAttr(kNameSpaceID_None, nsGkAtoms::target, target);
      aDocument->SetBaseTarget(target);
      return;
    }
  }

  aDocument->SetBaseTarget(EmptyString());
}

nsresult
HTMLSharedElement::SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify)
{
  nsresult rv = nsGenericHTMLElement::SetAttr(aNameSpaceID, aName, aPrefix,
                                              aValue, aNotify);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  if (mNodeInfo->Equals(nsGkAtoms::base) &&
      aNameSpaceID == kNameSpaceID_None &&
      IsInDoc()) {
    if (aName == nsGkAtoms::href) {
      SetBaseURIUsingFirstBaseWithHref(GetCurrentDoc(), this);
    } else if (aName == nsGkAtoms::target) {
      SetBaseTargetUsingFirstBaseWithTarget(GetCurrentDoc(), this);
    }
  }

  return NS_OK;
}

nsresult
HTMLSharedElement::UnsetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                             bool aNotify)
{
  nsresult rv = nsGenericHTMLElement::UnsetAttr(aNameSpaceID, aName, aNotify);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  if (mNodeInfo->Equals(nsGkAtoms::base) &&
      aNameSpaceID == kNameSpaceID_None &&
      IsInDoc()) {
    if (aName == nsGkAtoms::href) {
      SetBaseURIUsingFirstBaseWithHref(GetCurrentDoc(), nullptr);
    } else if (aName == nsGkAtoms::target) {
      SetBaseTargetUsingFirstBaseWithTarget(GetCurrentDoc(), nullptr);
    }
  }

  return NS_OK;
}

nsresult
HTMLSharedElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument, aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (mNodeInfo->Equals(nsGkAtoms::base) &&
      aDocument) {
    if (HasAttr(kNameSpaceID_None, nsGkAtoms::href)) {
      SetBaseURIUsingFirstBaseWithHref(aDocument, this);
    }
    if (HasAttr(kNameSpaceID_None, nsGkAtoms::target)) {
      SetBaseTargetUsingFirstBaseWithTarget(aDocument, this);
    }
  }

  return NS_OK;
}

void
HTMLSharedElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  nsIDocument* doc = GetCurrentDoc();

  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);

  
  
  if (doc && mNodeInfo->Equals(nsGkAtoms::base)) {
    if (HasAttr(kNameSpaceID_None, nsGkAtoms::href)) {
      SetBaseURIUsingFirstBaseWithHref(doc, nullptr);
    }
    if (HasAttr(kNameSpaceID_None, nsGkAtoms::target)) {
      SetBaseTargetUsingFirstBaseWithTarget(doc, nullptr);
    }
  }
}

nsMapRuleToAttributesFunc
HTMLSharedElement::GetAttributeMappingFunction() const
{
  if (mNodeInfo->Equals(nsGkAtoms::dir)) {
    return &DirectoryMapAttributesIntoRule;
  }

  return nsGenericHTMLElement::GetAttributeMappingFunction();
}

} 
} 
