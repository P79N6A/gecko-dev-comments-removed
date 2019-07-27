




#include "mozilla/dom/HTMLMenuItemElement.h"

#include "mozilla/BasicEvents.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/dom/HTMLMenuItemElementBinding.h"
#include "nsAttrValueInlines.h"
#include "nsContentUtils.h"


NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(MenuItem)

namespace mozilla {
namespace dom {


#define NS_CHECKED_IS_TOGGLED (1 << 2)
#define NS_ORIGINAL_CHECKED_VALUE (1 << 3)
#define NS_MENUITEM_TYPE(bits) ((bits) & ~( \
  NS_CHECKED_IS_TOGGLED | NS_ORIGINAL_CHECKED_VALUE))

enum CmdType                                                                 
{                                                                            
  CMD_TYPE_MENUITEM = 1,
  CMD_TYPE_CHECKBOX,
  CMD_TYPE_RADIO
};

static const nsAttrValue::EnumTable kMenuItemTypeTable[] = {
  { "menuitem", CMD_TYPE_MENUITEM },
  { "checkbox", CMD_TYPE_CHECKBOX },
  { "radio", CMD_TYPE_RADIO },
  { 0 }
};

static const nsAttrValue::EnumTable* kMenuItemDefaultType =
  &kMenuItemTypeTable[0];


class Visitor
{
public:
  Visitor() { }
  virtual ~Visitor() { }

  




  virtual bool Visit(HTMLMenuItemElement* aMenuItem) = 0;
};


class GetCheckedVisitor : public Visitor
{
public:
  explicit GetCheckedVisitor(HTMLMenuItemElement** aResult)
    : mResult(aResult)
    { }
  virtual bool Visit(HTMLMenuItemElement* aMenuItem)
  {
    if (aMenuItem->IsChecked()) {
      *mResult = aMenuItem;
      return false;
    }
    return true;
  }
protected:
  HTMLMenuItemElement** mResult;
};


class ClearCheckedVisitor : public Visitor
{
public:
  explicit ClearCheckedVisitor(HTMLMenuItemElement* aExcludeMenuItem)
    : mExcludeMenuItem(aExcludeMenuItem)
    { }
  virtual bool Visit(HTMLMenuItemElement* aMenuItem)
  {
    if (aMenuItem != mExcludeMenuItem && aMenuItem->IsChecked()) {
      aMenuItem->ClearChecked();
    }
    return true;
  }
protected:
  HTMLMenuItemElement* mExcludeMenuItem;
};



class GetCheckedDirtyVisitor : public Visitor
{
public:
  GetCheckedDirtyVisitor(bool* aCheckedDirty,
                         HTMLMenuItemElement* aExcludeMenuItem)
    : mCheckedDirty(aCheckedDirty),
      mExcludeMenuItem(aExcludeMenuItem)
    { }
  virtual bool Visit(HTMLMenuItemElement* aMenuItem)
  {
    if (aMenuItem == mExcludeMenuItem) {
      return true;
    }
    *mCheckedDirty = aMenuItem->IsCheckedDirty();
    return false;
  }
protected:
  bool* mCheckedDirty;
  HTMLMenuItemElement* mExcludeMenuItem;
};


class SetCheckedDirtyVisitor : public Visitor
{
public:
  SetCheckedDirtyVisitor()
    { }
  virtual bool Visit(HTMLMenuItemElement* aMenuItem)
  {
    aMenuItem->SetCheckedDirty();
    return true;
  }
};



class CombinedVisitor : public Visitor
{
public:
  CombinedVisitor(Visitor* aVisitor1, Visitor* aVisitor2)
    : mVisitor1(aVisitor1), mVisitor2(aVisitor2),
      mContinue1(true), mContinue2(true)
    { }
  virtual bool Visit(HTMLMenuItemElement* aMenuItem)
  {
    if (mContinue1) {
      mContinue1 = mVisitor1->Visit(aMenuItem);
    }
    if (mContinue2) {
      mContinue2 = mVisitor2->Visit(aMenuItem);
    }
    return mContinue1 || mContinue2;
  }
protected:
  Visitor* mVisitor1;
  Visitor* mVisitor2;
  bool mContinue1;
  bool mContinue2;
};


HTMLMenuItemElement::HTMLMenuItemElement(
  already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo, FromParser aFromParser)
  : nsGenericHTMLElement(aNodeInfo),
    mType(kMenuItemDefaultType->value),
    mParserCreating(false),
    mShouldInitChecked(false),
    mCheckedDirty(false),
    mChecked(false)
{
  mParserCreating = aFromParser;
}

HTMLMenuItemElement::~HTMLMenuItemElement()
{
}


NS_IMPL_ISUPPORTS_INHERITED(HTMLMenuItemElement, nsGenericHTMLElement,
                            nsIDOMHTMLMenuItemElement)


nsresult
HTMLMenuItemElement::Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const
{
  *aResult = nullptr;
  already_AddRefed<mozilla::dom::NodeInfo> ni = nsRefPtr<mozilla::dom::NodeInfo>(aNodeInfo).forget();
  nsRefPtr<HTMLMenuItemElement> it =
    new HTMLMenuItemElement(ni, NOT_FROM_PARSER);
  nsresult rv = const_cast<HTMLMenuItemElement*>(this)->CopyInnerTo(it);
  if (NS_SUCCEEDED(rv)) {
    switch (mType) {
      case CMD_TYPE_CHECKBOX:
      case CMD_TYPE_RADIO:
        if (mCheckedDirty) {
          
          
          it->mCheckedDirty = true;
          it->mChecked = mChecked;
        }
        break;
    }

    it.forget(aResult);
  }

  return rv;
}


NS_IMPL_ENUM_ATTR_DEFAULT_VALUE(HTMLMenuItemElement, Type, type,
                                kMenuItemDefaultType->tag)

NS_IMPL_STRING_ATTR_WITH_FALLBACK(HTMLMenuItemElement, Label, label, GetText)
NS_IMPL_URI_ATTR(HTMLMenuItemElement, Icon, icon)
NS_IMPL_BOOL_ATTR(HTMLMenuItemElement, Disabled, disabled)
NS_IMPL_BOOL_ATTR(HTMLMenuItemElement, DefaultChecked, checked)

NS_IMPL_STRING_ATTR(HTMLMenuItemElement, Radiogroup, radiogroup)

NS_IMETHODIMP
HTMLMenuItemElement::GetChecked(bool* aChecked)
{
  *aChecked = mChecked;
  return NS_OK;
}

NS_IMETHODIMP
HTMLMenuItemElement::SetChecked(bool aChecked)
{
  bool checkedChanged = mChecked != aChecked;

  mChecked = aChecked;

  if (mType == CMD_TYPE_RADIO) {
    if (checkedChanged) {
      if (mCheckedDirty) {
        ClearCheckedVisitor visitor(this);
        WalkRadioGroup(&visitor);
      } else {
        ClearCheckedVisitor visitor1(this);
        SetCheckedDirtyVisitor visitor2;
        CombinedVisitor visitor(&visitor1, &visitor2);
        WalkRadioGroup(&visitor);
      }
    } else if (!mCheckedDirty) {
      SetCheckedDirtyVisitor visitor;
      WalkRadioGroup(&visitor);
    }
  } else {
    mCheckedDirty = true;
  }

  return NS_OK;
}

nsresult
HTMLMenuItemElement::PreHandleEvent(EventChainPreVisitor& aVisitor)
{
  if (aVisitor.mEvent->message == NS_MOUSE_CLICK) {

    bool originalCheckedValue = false;
    switch (mType) {
      case CMD_TYPE_CHECKBOX:
        originalCheckedValue = mChecked;
        SetChecked(!originalCheckedValue);
        aVisitor.mItemFlags |= NS_CHECKED_IS_TOGGLED;
        break;
      case CMD_TYPE_RADIO:
        nsCOMPtr<nsIDOMHTMLMenuItemElement> selectedRadio = GetSelectedRadio();
        aVisitor.mItemData = selectedRadio;

        originalCheckedValue = mChecked;
        if (!originalCheckedValue) {
          SetChecked(true);
          aVisitor.mItemFlags |= NS_CHECKED_IS_TOGGLED;
        }
        break;
    }

    if (originalCheckedValue) {
      aVisitor.mItemFlags |= NS_ORIGINAL_CHECKED_VALUE;
    }

    
    aVisitor.mItemFlags |= mType;
  }

  return nsGenericHTMLElement::PreHandleEvent(aVisitor);
}

nsresult
HTMLMenuItemElement::PostHandleEvent(EventChainPostVisitor& aVisitor)
{
  
  if (aVisitor.mEvent->message == NS_MOUSE_CLICK &&
      aVisitor.mItemFlags & NS_CHECKED_IS_TOGGLED &&
      aVisitor.mEventStatus == nsEventStatus_eConsumeNoDefault) {
    bool originalCheckedValue =
      !!(aVisitor.mItemFlags & NS_ORIGINAL_CHECKED_VALUE);
    uint8_t oldType = NS_MENUITEM_TYPE(aVisitor.mItemFlags);

    nsCOMPtr<nsIDOMHTMLMenuItemElement> selectedRadio =
      do_QueryInterface(aVisitor.mItemData);
    if (selectedRadio) {
      selectedRadio->SetChecked(true);
      if (mType != CMD_TYPE_RADIO) {
        SetChecked(false);
      }
    } else if (oldType == CMD_TYPE_CHECKBOX) {
      SetChecked(originalCheckedValue);
    }
  }

  return NS_OK;
}

nsresult
HTMLMenuItemElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                                nsIContent* aBindingParent,
                                bool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument, aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);

  if (NS_SUCCEEDED(rv) && aDocument && mType == CMD_TYPE_RADIO) {
    AddedToRadioGroup();
  }

  return rv;
}

bool
HTMLMenuItemElement::ParseAttribute(int32_t aNamespaceID,
                                    nsIAtom* aAttribute,
                                    const nsAString& aValue,
                                    nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::type) {
      bool success = aResult.ParseEnumValue(aValue, kMenuItemTypeTable,
                                              false);
      if (success) {
        mType = aResult.GetEnumValue();
      } else {
        mType = kMenuItemDefaultType->value;
      }

      return success;
    }

    if (aAttribute == nsGkAtoms::radiogroup) {
      aResult.ParseAtom(aValue);
      return true;
    }
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

void
HTMLMenuItemElement::DoneCreatingElement()
{
  mParserCreating = false;

  if (mShouldInitChecked) {
    InitChecked();
    mShouldInitChecked = false;
  }
}

void
HTMLMenuItemElement::GetText(nsAString& aText)
{
  nsAutoString text;
  if (!nsContentUtils::GetNodeTextContent(this, false, text)) {
    NS_RUNTIMEABORT("OOM");
  }

  text.CompressWhitespace(true, true);
  aText = text;
}

nsresult
HTMLMenuItemElement::AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                  const nsAttrValue* aValue, bool aNotify)
{
  if (aNameSpaceID == kNameSpaceID_None) {
    if ((aName == nsGkAtoms::radiogroup || aName == nsGkAtoms::type) &&
        mType == CMD_TYPE_RADIO &&
        !mParserCreating) {
      if (IsInDoc() && GetParent()) {
        AddedToRadioGroup();
      }
    }

    
    
    if (aName == nsGkAtoms::checked &&
        !mCheckedDirty) {
      if (mParserCreating) {
        mShouldInitChecked = true;
      } else {
        InitChecked();
      }
    }
  }

  return nsGenericHTMLElement::AfterSetAttr(aNameSpaceID, aName, aValue,
                                            aNotify);
}

void
HTMLMenuItemElement::WalkRadioGroup(Visitor* aVisitor)
{
  nsIContent* parent = GetParent();
  if (!parent) {
    aVisitor->Visit(this);
    return;
  }

  nsAttrInfo info1(GetAttrInfo(kNameSpaceID_None,
                               nsGkAtoms::radiogroup));
  bool info1Empty = !info1.mValue || info1.mValue->IsEmptyString();

  for (nsIContent* cur = parent->GetFirstChild();
       cur;
       cur = cur->GetNextSibling()) {
    HTMLMenuItemElement* menuitem = HTMLMenuItemElement::FromContent(cur);

    if (!menuitem || menuitem->GetType() != CMD_TYPE_RADIO) {
      continue;
    }

    nsAttrInfo info2(menuitem->GetAttrInfo(kNameSpaceID_None,
                                           nsGkAtoms::radiogroup));
    bool info2Empty = !info2.mValue || info2.mValue->IsEmptyString();

    if (info1Empty != info2Empty ||
        (info1.mValue && info2.mValue && !info1.mValue->Equals(*info2.mValue))) {
      continue;
    }

    if (!aVisitor->Visit(menuitem)) {
      break;
    }
  }
}

HTMLMenuItemElement*
HTMLMenuItemElement::GetSelectedRadio()
{
  HTMLMenuItemElement* result = nullptr;

  GetCheckedVisitor visitor(&result);
  WalkRadioGroup(&visitor);

  return result;
}

void
HTMLMenuItemElement::AddedToRadioGroup()
{
  bool checkedDirty = mCheckedDirty;
  if (mChecked) {
    ClearCheckedVisitor visitor1(this);
    GetCheckedDirtyVisitor visitor2(&checkedDirty, this);
    CombinedVisitor visitor(&visitor1, &visitor2);
    WalkRadioGroup(&visitor);
  } else {
    GetCheckedDirtyVisitor visitor(&checkedDirty, this);
    WalkRadioGroup(&visitor);
  }
  mCheckedDirty = checkedDirty;
}

void
HTMLMenuItemElement::InitChecked()
{
  bool defaultChecked;
  GetDefaultChecked(&defaultChecked);
  mChecked = defaultChecked;
  if (mType == CMD_TYPE_RADIO) {
    ClearCheckedVisitor visitor(this);
    WalkRadioGroup(&visitor);
  }
}

JSObject*
HTMLMenuItemElement::WrapNode(JSContext* aCx)
{
  return HTMLMenuItemElementBinding::Wrap(aCx, this);
}

} 
} 

#undef NS_ORIGINAL_CHECKED_VALUE
