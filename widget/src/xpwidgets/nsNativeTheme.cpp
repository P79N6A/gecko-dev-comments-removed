





































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

PRUint8                   nsNativeTheme::sButtonActiveBorderStyle = NS_STYLE_BORDER_STYLE_INSET;
PRUint8                   nsNativeTheme::sButtonInactiveBorderStyle = NS_STYLE_BORDER_STYLE_OUTSET;
nsILookAndFeel::nsColorID nsNativeTheme::sButtonBorderColorID = nsILookAndFeel::eColor_buttonface;
nsILookAndFeel::nsColorID nsNativeTheme::sButtonDisabledBorderColorID = nsILookAndFeel::eColor_buttonface;
nsILookAndFeel::nsColorID nsNativeTheme::sButtonBGColorID = nsILookAndFeel::eColor_buttonface;
nsILookAndFeel::nsColorID nsNativeTheme::sButtonDisabledBGColorID = nsILookAndFeel::eColor_buttonface;

PRUint8                   nsNativeTheme::sTextfieldBorderStyle = NS_STYLE_BORDER_STYLE_INSET;
nsILookAndFeel::nsColorID nsNativeTheme::sTextfieldBorderColorID = nsILookAndFeel::eColor_threedface;
PRBool                    nsNativeTheme::sTextfieldBGTransparent = PR_FALSE;
nsILookAndFeel::nsColorID nsNativeTheme::sTextfieldBGColorID = nsILookAndFeel::eColor__moz_field;
nsILookAndFeel::nsColorID nsNativeTheme::sTextfieldDisabledBGColorID = nsILookAndFeel::eColor_threedface;

PRUint8                   nsNativeTheme::sListboxBorderStyle = NS_STYLE_BORDER_STYLE_INSET;
nsILookAndFeel::nsColorID nsNativeTheme::sListboxBorderColorID = nsILookAndFeel::eColor_threedface;
nsILookAndFeel::nsColorID nsNativeTheme::sListboxBGColorID = nsILookAndFeel::eColor__moz_field;
nsILookAndFeel::nsColorID nsNativeTheme::sListboxDisabledBGColorID = nsILookAndFeel::eColor_threedface;

PRUint8                   nsNativeTheme::sComboboxBorderStyle = NS_STYLE_BORDER_STYLE_INSET;
nsILookAndFeel::nsColorID nsNativeTheme::sComboboxBorderColorID = nsILookAndFeel::eColor_threedface;
PRBool                    nsNativeTheme::sComboboxBGTransparent = PR_FALSE;
nsILookAndFeel::nsColorID nsNativeTheme::sComboboxBGColorID = nsILookAndFeel::eColor__moz_field;
nsILookAndFeel::nsColorID nsNativeTheme::sComboboxDisabledBGColorID = nsILookAndFeel::eColor_threedface;

#ifdef MOZ_WIDGET_GTK2

nsMargin                  nsNativeTheme::sTextfieldBorderSize(3, 3, 3, 3);
nsMargin                  nsNativeTheme::sListboxBorderSize(3, 3, 3, 3);
nsMargin                  nsNativeTheme::sComboboxBorderSize(3, 3, 3, 3);
nsMargin                  nsNativeTheme::sButtonBorderSize(3, 3, 3, 3);
#else
nsMargin                  nsNativeTheme::sTextfieldBorderSize(2, 2, 2, 2);
nsMargin                  nsNativeTheme::sListboxBorderSize(2, 2, 2, 2);
nsMargin                  nsNativeTheme::sComboboxBorderSize(2, 2, 2, 2);
nsMargin                  nsNativeTheme::sButtonBorderSize(2, 2, 2, 2);
#endif

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
nsNativeTheme::CheckIntAttr(nsIFrame* aFrame, nsIAtom* aAtom)
{
  if (!aFrame)
    return 0;

  nsAutoString attr;
  aFrame->GetContent()->GetAttr(kNameSpaceID_None, aAtom, attr);
  PRInt32 err, value = attr.ToInteger(&err);
  if (NS_FAILED(err))
    return 0;

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

static void
ConvertBorderToAppUnits(nsPresContext* aPresContext, const nsMargin &aSource, nsMargin &aDest)
{
  PRInt32 cp2a = nsPresContext::AppUnitsPerCSSPixel();
  PRInt32 dp2a = aPresContext->AppUnitsPerDevPixel();
  aDest.top = NS_ROUND_BORDER_TO_PIXELS(NSIntPixelsToAppUnits(aSource.top, cp2a), dp2a);
  aDest.left = NS_ROUND_BORDER_TO_PIXELS(NSIntPixelsToAppUnits(aSource.left, cp2a), dp2a);
  aDest.bottom = NS_ROUND_BORDER_TO_PIXELS(NSIntPixelsToAppUnits(aSource.bottom, cp2a), dp2a);
  aDest.right = NS_ROUND_BORDER_TO_PIXELS(NSIntPixelsToAppUnits(aSource.right, cp2a), dp2a);;
}

PRBool
nsNativeTheme::IsWidgetStyled(nsPresContext* aPresContext, nsIFrame* aFrame,
                              PRUint8 aWidgetType)
{
  
  if (aFrame && (aWidgetType == NS_THEME_BUTTON ||
                 aWidgetType == NS_THEME_TEXTFIELD ||
                 aWidgetType == NS_THEME_TEXTFIELD_MULTILINE ||
                 aWidgetType == NS_THEME_LISTBOX ||
                 aWidgetType == NS_THEME_DROPDOWN)) {
    if (aFrame->GetContent()->IsNodeOfType(nsINode::eHTML)) {
      nscolor defaultBGColor;
      nscolor defaultBorderColor;
      PRUint8 defaultBorderStyle;
      nsMargin defaultBorderSize;
      PRBool defaultBGTransparent = PR_FALSE;

      nsILookAndFeel *lookAndFeel = aPresContext->LookAndFeel();

      switch (aWidgetType) {
      case NS_THEME_BUTTON:
        ConvertBorderToAppUnits(aPresContext, sButtonBorderSize, 
                                defaultBorderSize);
        if (IsDisabled(aFrame)) {
          defaultBorderStyle = sButtonInactiveBorderStyle;
          lookAndFeel->GetColor(sButtonDisabledBorderColorID,
                                defaultBorderColor);
          lookAndFeel->GetColor(sButtonDisabledBGColorID,
                                defaultBGColor);
        } else {
          PRInt32 contentState = GetContentState(aFrame, aWidgetType);
          if (contentState & NS_EVENT_STATE_HOVER &&
              contentState & NS_EVENT_STATE_ACTIVE)
            defaultBorderStyle = sButtonActiveBorderStyle;
          else
            defaultBorderStyle = sButtonInactiveBorderStyle;
          lookAndFeel->GetColor(sButtonBorderColorID,
                                defaultBorderColor);
          lookAndFeel->GetColor(sButtonBGColorID,
                                defaultBGColor);
        }
        break;

      case NS_THEME_TEXTFIELD:
      case NS_THEME_TEXTFIELD_MULTILINE:
        defaultBorderStyle = sTextfieldBorderStyle;
        ConvertBorderToAppUnits(aPresContext, sTextfieldBorderSize, defaultBorderSize);
        lookAndFeel->GetColor(sTextfieldBorderColorID, defaultBorderColor);
        defaultBGTransparent = sTextfieldBGTransparent;
        if (!defaultBGTransparent) {
          if (IsDisabled(aFrame))
            lookAndFeel->GetColor(sTextfieldDisabledBGColorID, defaultBGColor);
          else
            lookAndFeel->GetColor(sTextfieldBGColorID, defaultBGColor);
        }
        break;

      case NS_THEME_LISTBOX:
        defaultBorderStyle = sListboxBorderStyle;
        ConvertBorderToAppUnits(aPresContext, sListboxBorderSize, defaultBorderSize);
        lookAndFeel->GetColor(sListboxBorderColorID, defaultBorderColor);
        if (IsDisabled(aFrame))
          lookAndFeel->GetColor(sListboxDisabledBGColorID, defaultBGColor);
        else
          lookAndFeel->GetColor(sListboxBGColorID, defaultBGColor);
        break;

      case NS_THEME_DROPDOWN:
        defaultBorderStyle = sComboboxBorderStyle;
        ConvertBorderToAppUnits(aPresContext, sComboboxBorderSize, defaultBorderSize);
        lookAndFeel->GetColor(sComboboxBorderColorID, defaultBorderColor);
        defaultBGTransparent = sComboboxBGTransparent;
        if (!defaultBGTransparent) {
          if (IsDisabled(aFrame))
            lookAndFeel->GetColor(sComboboxDisabledBGColorID, defaultBGColor);
          else
            lookAndFeel->GetColor(sComboboxBGColorID, defaultBGColor);
        }
        break;

      default:
        NS_ERROR("nsNativeTheme::IsWidgetStyled widget type not handled");
        return PR_FALSE;
      }

      
      const nsStyleBackground* ourBG = aFrame->GetStyleBackground();

      if (defaultBGTransparent) {
        if (!(ourBG->mBackgroundFlags & NS_STYLE_BG_COLOR_TRANSPARENT))
          return PR_TRUE;
      } else if (ourBG->mBackgroundColor != defaultBGColor ||
                 ourBG->mBackgroundFlags & NS_STYLE_BG_COLOR_TRANSPARENT)
        return PR_TRUE;

      if (!(ourBG->mBackgroundFlags & NS_STYLE_BG_IMAGE_NONE))
        return PR_TRUE;

      
      const nsStyleBorder* ourBorder = aFrame->GetStyleBorder();

      for (PRInt32 i = 0; i < 4; ++i) {
        if (ourBorder->GetBorderStyle(i) != defaultBorderStyle)
          return PR_TRUE;

        PRBool borderFG, borderClear;
        nscolor borderColor;
        ourBorder->GetBorderColor(i, borderColor, borderFG, borderClear);
        if (borderColor != defaultBorderColor || borderClear)
          return PR_TRUE;
      }

      
      if (ourBorder->GetBorder() != defaultBorderSize)
        return PR_TRUE;
    }
  }

  return PR_FALSE;
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
nsNativeTheme::IsIndeterminateProgress(nsIFrame* aFrame)
{
  if (!aFrame)
    return PR_FALSE;

  return aFrame->GetContent()->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::mode,
                                           NS_LITERAL_STRING("undetermined"),
                                           eCaseMatters);
}
