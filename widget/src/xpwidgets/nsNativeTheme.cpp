





































#include "nsNativeTheme.h"
#include "nsIWidget.h"
#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsEventStateManager.h"
#include "nsString.h"
#include "nsINameSpaceManager.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsILookAndFeel.h"
#include "nsThemeConstants.h"
#include "nsIComponentManager.h"
#include "nsPIDOMWindow.h"

nsNativeTheme::nsNativeTheme()
: mAnimatedContentTimeout(PR_UINT32_MAX)
{
}

NS_IMPL_ISUPPORTS1(nsNativeTheme, nsITimerCallback)

nsIPresShell *
nsNativeTheme::GetPresShell(nsIFrame* aFrame)
{
  if (!aFrame)
    return nsnull;

  
  
  nsPresContext *context = aFrame->GetStyleContext()->GetRuleNode()->GetPresContext();
  return context ? context->GetPresShell() : nsnull;
}

nsEventStates
nsNativeTheme::GetContentState(nsIFrame* aFrame, PRUint8 aWidgetType)
{
  if (!aFrame)
    return nsEventStates();

  PRBool isXULCheckboxRadio = 
    (aWidgetType == NS_THEME_CHECKBOX ||
     aWidgetType == NS_THEME_RADIO) &&
    aFrame->GetContent()->IsXUL();
  if (isXULCheckboxRadio)
    aFrame = aFrame->GetParent();

  if (!aFrame->GetContent())
    return nsEventStates();

  nsIPresShell *shell = GetPresShell(aFrame);
  if (!shell)
    return nsEventStates();

  nsEventStateManager* esm = shell->GetPresContext()->EventStateManager();
  nsEventStates flags = esm->GetContentState(aFrame->GetContent(), PR_TRUE);
  
  if (isXULCheckboxRadio && aWidgetType == NS_THEME_RADIO) {
    if (IsFocused(aFrame))
      flags |= NS_EVENT_STATE_FOCUS;
  }

  
  
  
#if defined(XP_MACOSX)
  
  if (aWidgetType == NS_THEME_TEXTFIELD ||
      aWidgetType == NS_THEME_TEXTFIELD_MULTILINE ||
      aWidgetType == NS_THEME_SEARCHFIELD ||
      aWidgetType == NS_THEME_LISTBOX) {
    return flags;
  }
#endif
#if defined(XP_WIN)
  
  if (aWidgetType == NS_THEME_BUTTON)
    return flags;
#endif    
#if defined(XP_MACOSX) || defined(XP_WIN)
  nsIDocument* doc = aFrame->GetContent()->GetOwnerDoc();
  if (doc) {
    nsPIDOMWindow* window = doc->GetWindow();
    if (window && !window->ShouldShowFocusRing())
      flags &= ~NS_EVENT_STATE_FOCUS;
  }
#endif
  
  return flags;
}

PRBool
nsNativeTheme::CheckBooleanAttr(nsIFrame* aFrame, nsIAtom* aAtom)
{
  if (!aFrame)
    return PR_FALSE;

  nsIContent* content = aFrame->GetContent();
  if (!content)
    return PR_FALSE;

  if (content->IsHTML())
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

  if (content->IsXUL()) {
    
    
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
nsNativeTheme::IsButtonTypeMenu(nsIFrame* aFrame)
{
  if (!aFrame)
    return PR_FALSE;

  nsIContent* content = aFrame->GetContent();
  return content->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::type,
                              NS_LITERAL_STRING("menu"), eCaseMatters);
}

PRBool
nsNativeTheme::GetIndeterminate(nsIFrame* aFrame)
{
  if (!aFrame)
    return PR_FALSE;

  nsIContent* content = aFrame->GetContent();

  if (content->IsXUL()) {
    
    
    return CheckBooleanAttr(aFrame->GetParent(), nsWidgetAtoms::indeterminate);
  }

  
  nsCOMPtr<nsIDOMHTMLInputElement> inputElt = do_QueryInterface(content);
  if (inputElt) {
    PRBool indeterminate;
    inputElt->GetIndeterminate(&indeterminate);
    return indeterminate;
  }

  return PR_FALSE;
}

PRBool
nsNativeTheme::IsWidgetStyled(nsPresContext* aPresContext, nsIFrame* aFrame,
                              PRUint8 aWidgetType)
{
  
  if (!aFrame)
    return PR_FALSE;

  
  
  
  
  
  if (aWidgetType == NS_THEME_RESIZER) {
    nsIFrame* parentFrame = aFrame->GetParent();
    if (parentFrame && parentFrame->GetType() == nsWidgetAtoms::scrollFrame) {
      
      
      parentFrame = parentFrame->GetParent();
      if (parentFrame) {
        return IsWidgetStyled(aPresContext, parentFrame,
                              parentFrame->GetStyleDisplay()->mAppearance);
      }
    }
  }

  return (aWidgetType == NS_THEME_BUTTON ||
          aWidgetType == NS_THEME_TEXTFIELD ||
          aWidgetType == NS_THEME_TEXTFIELD_MULTILINE ||
          aWidgetType == NS_THEME_LISTBOX ||
          aWidgetType == NS_THEME_DROPDOWN) &&
         aFrame->GetContent()->IsHTML() &&
         aPresContext->HasAuthorSpecifiedRules(aFrame,
                                               NS_AUTHOR_SPECIFIED_BORDER |
                                               NS_AUTHOR_SPECIFIED_BACKGROUND);
}

bool
nsNativeTheme::IsDisabled(nsIFrame* aFrame, nsEventStates aEventStates)
{
  if (!aFrame) {
    return false;
  }

  nsIContent* content = aFrame->GetContent();
  if (!content) {
    return PR_FALSE;
  }

  if (content->IsHTML()) {
    return aEventStates.HasState(NS_EVENT_STATE_DISABLED);
  }

  
  
  
  return content->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::disabled,
                              NS_LITERAL_STRING("true"), eCaseMatters);
}

PRBool
nsNativeTheme::IsFrameRTL(nsIFrame* aFrame)
{
  return aFrame && aFrame->GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL;
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
  if (!aFrame || !aFrame->GetContent())
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
nsNativeTheme::IsLastTreeHeaderCell(nsIFrame* aFrame)
{
  if (!aFrame)
    return PR_FALSE;

  
  if (aFrame->GetContent()->Tag() == nsWidgetAtoms::treecolpicker)
    return PR_TRUE;

  
  nsIContent* parent = aFrame->GetContent()->GetParent();
  while (parent && parent->Tag() != nsWidgetAtoms::tree) {
    parent = parent->GetParent();
  }

  
  if (parent && !parent->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::hidecolumnpicker,
                                     NS_LITERAL_STRING("true"), eCaseMatters))
    return PR_FALSE;

  while ((aFrame = aFrame->GetNextSibling())) {
    if (aFrame->GetRect().width > 0)
      return PR_FALSE;
  }
  return PR_TRUE;
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

  nsIFrame* first = aFrame->GetParent()->GetFirstChild(nsnull);
  while (first) {
    if (first->GetRect().width > 0 && first->GetContent()->Tag() == nsWidgetAtoms::tab)
      return (first == aFrame);
    first = first->GetNextSibling();
  }
  return PR_FALSE;
}

PRBool
nsNativeTheme::IsLastTab(nsIFrame* aFrame)
{
  if (!aFrame)
    return PR_FALSE;

  while ((aFrame = aFrame->GetNextSibling())) {
    if (aFrame->GetRect().width > 0 && aFrame->GetContent()->Tag() == nsWidgetAtoms::tab)
      return PR_FALSE;
  }
  return PR_TRUE;
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
nsNativeTheme::IsNextToSelectedTab(nsIFrame* aFrame, PRInt32 aOffset)
{
  if (!aFrame)
    return PR_FALSE;

  if (aOffset == 0)
    return IsSelectedTab(aFrame);

  PRInt32 thisTabIndex = -1, selectedTabIndex = -1;

  nsIFrame* currentTab = aFrame->GetParent()->GetFirstChild(NULL);
  for (PRInt32 i = 0; currentTab; currentTab = currentTab->GetNextSibling()) {
    if (currentTab->GetRect().width == 0)
      continue;
    if (aFrame == currentTab)
      thisTabIndex = i;
    if (IsSelectedTab(currentTab))
      selectedTabIndex = i;
    ++i;
  }

  if (thisTabIndex == -1 || selectedTabIndex == -1)
    return PR_FALSE;

  return (thisTabIndex - selectedTabIndex == aOffset);
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


PRBool
nsNativeTheme::IsSubmenu(nsIFrame* aFrame, PRBool* aLeftOfParent)
{
  if (!aFrame)
    return PR_FALSE;

  nsIContent* parentContent = aFrame->GetContent()->GetParent();
  if (!parentContent || parentContent->Tag() != nsWidgetAtoms::menu)
    return PR_FALSE;

  nsIFrame* parent = aFrame;
  while ((parent = parent->GetParent())) {
    if (parent->GetContent() == parentContent) {
      if (aLeftOfParent) {
        nsIntRect selfBounds, parentBounds;
        aFrame->GetNearestWidget()->GetScreenBounds(selfBounds);
        parent->GetNearestWidget()->GetScreenBounds(parentBounds);
        *aLeftOfParent = selfBounds.x < parentBounds.x;
      }
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

PRBool
nsNativeTheme::QueueAnimatedContentForRefresh(nsIContent* aContent,
                                              PRUint32 aMinimumFrameRate)
{
  NS_ASSERTION(aContent, "Null pointer!");
  NS_ASSERTION(aMinimumFrameRate, "aMinimumFrameRate must be non-zero!");
  NS_ASSERTION(aMinimumFrameRate <= 1000,
               "aMinimumFrameRate must be less than 1000!");

  PRUint32 timeout = PRUint32(NS_floor(1000 / aMinimumFrameRate));
  timeout = PR_MIN(mAnimatedContentTimeout, timeout);

  if (!mAnimatedContentTimer) {
    mAnimatedContentTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
    NS_ENSURE_TRUE(mAnimatedContentTimer, PR_FALSE);
  }

  if (mAnimatedContentList.IsEmpty() || timeout != mAnimatedContentTimeout) {
    nsresult rv;
    if (!mAnimatedContentList.IsEmpty()) {
      rv = mAnimatedContentTimer->Cancel();
      NS_ENSURE_SUCCESS(rv, PR_FALSE);
    }

    rv = mAnimatedContentTimer->InitWithCallback(this, timeout,
                                                 nsITimer::TYPE_ONE_SHOT);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    mAnimatedContentTimeout = timeout;
  }

  if (!mAnimatedContentList.AppendElement(aContent)) {
    NS_WARNING("Out of memory!");
    return PR_FALSE;
  }

  return PR_TRUE;
}

NS_IMETHODIMP
nsNativeTheme::Notify(nsITimer* aTimer)
{
  NS_ASSERTION(aTimer == mAnimatedContentTimer, "Wrong timer!");

  
  

  PRUint32 count = mAnimatedContentList.Length();
  for (PRUint32 index = 0; index < count; index++) {
    nsIFrame* frame = mAnimatedContentList[index]->GetPrimaryFrame();
    if (frame) {
#ifdef MOZ_ENABLE_LIBXUL
      frame->InvalidateOverflowRect();
#else
      frame->InvalidateOverflowRectExternal();
#endif
    }
  }

  mAnimatedContentList.Clear();
  mAnimatedContentTimeout = PR_UINT32_MAX;
  return NS_OK;
}
