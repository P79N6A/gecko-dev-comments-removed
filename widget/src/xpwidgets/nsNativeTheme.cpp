





































#include "nsNativeTheme.h"
#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIEventStateManager.h"
#include "nsString.h"
#include "nsINameSpaceManager.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsILookAndFeel.h"
#include "nsThemeConstants.h"
#include "nsIComponentManager.h"

nsNativeTheme::nsNativeTheme()
{
}

nsIPresShell *
nsNativeTheme::GetPresShell(nsIFrame* aFrame)
{
  if (!aFrame)
    return nsnull;

  
  
  nsPresContext *context = aFrame->GetStyleContext()->GetRuleNode()->GetPresContext();
  return context ? context->GetPresShell() : nsnull;
}

PRInt32
nsNativeTheme::GetContentState(nsIFrame* aFrame, PRUint8 aWidgetType)
{
  if (!aFrame)
    return 0;

  PRBool isXULCheckboxRadio = 
    (aWidgetType == NS_THEME_CHECKBOX ||
     aWidgetType == NS_THEME_CHECKBOX_SMALL ||
     aWidgetType == NS_THEME_RADIO ||
     aWidgetType == NS_THEME_RADIO_SMALL) &&
    aFrame->GetContent()->IsNodeOfType(nsINode::eXUL);
  if (isXULCheckboxRadio)
    aFrame = aFrame->GetParent();

  nsIPresShell *shell = GetPresShell(aFrame);
  if (!shell)
    return 0;

  PRInt32 flags = 0;
  shell->GetPresContext()->EventStateManager()->GetContentState(aFrame->GetContent(), flags);
  
  if (isXULCheckboxRadio && (aWidgetType == NS_THEME_RADIO || aWidgetType == NS_THEME_RADIO_SMALL)) {
    if (IsFocused(aFrame))
      flags |= NS_EVENT_STATE_FOCUS;
  }
  
  return flags;
}

PRBool
nsNativeTheme::CheckBooleanAttr(nsIFrame* aFrame, nsIAtom* aAtom)
{
  if (!aFrame)
    return PR_FALSE;

  nsIContent* content = aFrame->GetContent();
  if (content->IsNodeOfType(nsINode::eHTML))
    return content->HasAttr(kNameSpaceID_None, aAtom);

  
  
  
  return content->AttrValueIs(kNameSpaceID_None, aAtom,
                              NS_LITERAL_STRING("true"), eCaseMatters);
}

PRInt32
nsNativeTheme::CheckIntAttr(nsIFrame* aFrame, nsIAtom* aAtom, PRInt32 defaultValue)
{
  if (!aFrame)
    return defaultValue;

  nsAutoString attr;
  aFrame->GetContent()->GetAttr(kNameSpaceID_None, aAtom, attr);
  PRInt32 err, value = attr.ToInteger(&err);
  if (attr.IsEmpty() || NS_FAILED(err))
    return defaultValue;

  return value;
}

PRBool
nsNativeTheme::GetCheckedOrSelected(nsIFrame* aFrame, PRBool aCheckSelected)
{
  if (!aFrame)
    return PR_FALSE;

  nsIContent* content = aFrame->GetContent();

  if (content->IsNodeOfType(nsINode::eXUL)) {
    
    
    aFrame = aFrame->GetParent();
  } else {
    
    nsCOMPtr<nsIDOMHTMLInputElement> inputElt = do_QueryInterface(content);
    if (inputElt) {
      PRBool checked;
      inputElt->GetChecked(&checked);
      return checked;
    }
  }

  return CheckBooleanAttr(aFrame, aCheckSelected ? nsWidgetAtoms::selected
                                                 : nsWidgetAtoms::checked);
}

PRBool
nsNativeTheme::IsWidgetStyled(nsPresContext* aPresContext, nsIFrame* aFrame,
                              PRUint8 aWidgetType)
{
  
  return aFrame &&
         (aWidgetType == NS_THEME_BUTTON ||
          aWidgetType == NS_THEME_TEXTFIELD ||
          aWidgetType == NS_THEME_TEXTFIELD_MULTILINE ||
          aWidgetType == NS_THEME_LISTBOX ||
          aWidgetType == NS_THEME_DROPDOWN) &&
         aFrame->GetContent()->IsNodeOfType(nsINode::eHTML) &&
         aPresContext->HasAuthorSpecifiedRules(aFrame,
                                               NS_AUTHOR_SPECIFIED_BORDER |
                                               NS_AUTHOR_SPECIFIED_BACKGROUND);
}


PRInt32
nsNativeTheme::GetScrollbarButtonType(nsIFrame* aFrame)
{
  if (!aFrame)
    return 0;

  static nsIContent::AttrValuesArray strings[] =
    {&nsWidgetAtoms::scrollbarDownBottom, &nsWidgetAtoms::scrollbarDownTop,
     &nsWidgetAtoms::scrollbarUpBottom, &nsWidgetAtoms::scrollbarUpTop,
     nsnull};

  switch (aFrame->GetContent()->FindAttrValueIn(kNameSpaceID_None,
                                                nsWidgetAtoms::sbattr,
                                                strings, eCaseMatters)) {
    case 0: return eScrollbarButton_Down | eScrollbarButton_Bottom;
    case 1: return eScrollbarButton_Down;
    case 2: return eScrollbarButton_Bottom;
    case 3: return eScrollbarButton_UpTop;
  }

  return 0;
}


nsNativeTheme::TreeSortDirection
nsNativeTheme::GetTreeSortDirection(nsIFrame* aFrame)
{
  if (!aFrame)
    return eTreeSortDirection_Natural;

  static nsIContent::AttrValuesArray strings[] =
    {&nsWidgetAtoms::descending, &nsWidgetAtoms::ascending, nsnull};
  switch (aFrame->GetContent()->FindAttrValueIn(kNameSpaceID_None,
                                                nsWidgetAtoms::sortdirection,
                                                strings, eCaseMatters)) {
    case 0: return eTreeSortDirection_Descending;
    case 1: return eTreeSortDirection_Ascending;
  }

  return eTreeSortDirection_Natural;
}


PRBool
nsNativeTheme::IsBottomTab(nsIFrame* aFrame)
{
  if (!aFrame)
    return PR_FALSE;

  nsAutoString classStr;
  aFrame->GetContent()->GetAttr(kNameSpaceID_None, nsWidgetAtoms::_class, classStr);
  return !classStr.IsEmpty() && classStr.Find("tab-bottom") != kNotFound;
}

PRBool
nsNativeTheme::IsFirstTab(nsIFrame* aFrame)
{
  if (!aFrame)
    return PR_FALSE;

  return aFrame->GetContent()->HasAttr(kNameSpaceID_None, nsWidgetAtoms::firsttab);
}

PRBool
nsNativeTheme::IsLastTab(nsIFrame* aFrame)
{
  if (!aFrame)
    return PR_FALSE;

  return aFrame->GetContent()->HasAttr(kNameSpaceID_None, nsWidgetAtoms::lasttab);
}

PRBool
nsNativeTheme::IsHorizontal(nsIFrame* aFrame)
{
  if (!aFrame)
    return PR_FALSE;
    
  return !aFrame->GetContent()->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::orient,
                                            nsWidgetAtoms::vertical, 
                                            eCaseMatters);
}


PRBool
nsNativeTheme::IsIndeterminateProgress(nsIFrame* aFrame)
{
  if (!aFrame)
    return PR_FALSE;

  return aFrame->GetContent()->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::mode,
                                           NS_LITERAL_STRING("undetermined"),
                                           eCaseMatters);
}
