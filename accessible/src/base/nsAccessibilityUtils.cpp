





































#include "nsAccessibilityUtils.h"
#include "nsIDOMXULSelectCntrlEl.h"
#include "nsIDOMXULSelectCntrlItemEl.h"

void
nsAccessibilityUtils::GetAccAttr(nsIPersistentProperties *aAttributes,
                                 nsIAtom *aAttrName,
                                 nsAString& aAttrValue)
{
  nsCAutoString attrName;
  aAttrName->ToUTF8String(attrName);
  aAttributes->GetStringProperty(attrName, aAttrValue);
}

void
nsAccessibilityUtils::SetAccAttr(nsIPersistentProperties *aAttributes,
                                 nsIAtom *aAttrName,
                                 const nsAString& aAttrValue)
{
  nsAutoString oldValue;
  nsCAutoString attrName;

  aAttrName->ToUTF8String(attrName);
  aAttributes->SetStringProperty(attrName, aAttrValue, oldValue);
}

void
nsAccessibilityUtils::GetAccGroupAttrs(nsIPersistentProperties *aAttributes,
                                       PRInt32 *aLevel,
                                       PRInt32 *aPosInSet,
                                       PRInt32 *aSizeSet)
{
  *aLevel = 0;
  *aPosInSet = 0;
  *aSizeSet = 0;

  nsAutoString value;
  PRInt32 error = NS_OK;

  GetAccAttr(aAttributes, nsAccessibilityAtoms::level, value);
  if (!value.IsEmpty()) {
    PRInt32 level = value.ToInteger(&error);
    if (NS_SUCCEEDED(error))
      *aLevel = level;
  }

  GetAccAttr(aAttributes, nsAccessibilityAtoms::setsize, value);
  if (!value.IsEmpty()) {
    PRInt32 posInSet = value.ToInteger(&error);
    if (NS_SUCCEEDED(error))
      *aPosInSet = posInSet;
  }

  GetAccAttr(aAttributes, nsAccessibilityAtoms::posinset, value);
  if (!value.IsEmpty()) {
    PRInt32 sizeSet = value.ToInteger(&error);
    if (NS_SUCCEEDED(error))
      *aSizeSet = sizeSet;
  }
}

void
nsAccessibilityUtils::SetAccGroupAttrs(nsIPersistentProperties *aAttributes,
                                       PRInt32 aLevel,
                                       PRInt32 aPosInSet,
                                       PRInt32 aSizeSet)
{
  nsAutoString value;

  if (aLevel) {
    value.AppendInt(aLevel);
    SetAccAttr(aAttributes, nsAccessibilityAtoms::level, value);
  }

  if (aSizeSet && aPosInSet) {
    value.Truncate();
    value.AppendInt(aPosInSet);
    SetAccAttr(aAttributes, nsAccessibilityAtoms::posinset, value);

    value.Truncate();
    value.AppendInt(aSizeSet);
    SetAccAttr(aAttributes, nsAccessibilityAtoms::setsize, value);
  }
}

void
nsAccessibilityUtils::SetAccAttrsForXULSelectControlItem(nsIDOMNode *aNode,
                                                         nsIPersistentProperties *aAttributes)
{
  nsCOMPtr<nsIDOMXULSelectControlItemElement> item(do_QueryInterface(aNode));
  if (!item)
    return;

  nsCOMPtr<nsIDOMXULSelectControlElement> control;
  item->GetControl(getter_AddRefs(control));
  if (!control)
    return;

  PRUint32 itemsCount;
  control->GetItemCount(&itemsCount);
  PRInt32 indexOf;
  control->GetIndexOfItem(item, &indexOf);

  SetAccGroupAttrs(aAttributes, 0, indexOf + 1, itemsCount);
}
