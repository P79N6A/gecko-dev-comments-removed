






































#include "nsBoxLayoutState.h"
#include "nsBox.h"
#include "nsBoxFrame.h"
#include "nsPresContext.h"
#include "nsCOMPtr.h"
#include "nsIContent.h"
#include "nsHTMLContainerFrame.h"
#include "nsINameSpaceManager.h"
#include "nsGkAtoms.h"
#include "nsFrameManager.h"
#include "nsIDOMNode.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMAttr.h"
#include "nsIDocument.h"
#include "nsITheme.h"
#include "nsIServiceManager.h"
#include "nsIBoxLayout.h"
#include "FrameLayerBuilder.h"

using namespace mozilla;

#ifdef DEBUG_LAYOUT
PRInt32 gIndent = 0;
#endif

#ifdef DEBUG_LAYOUT
void
nsBoxAddIndents()
{
    for(PRInt32 i=0; i < gIndent; i++)
    {
        printf(" ");
    }
}
#endif

#ifdef DEBUG_LAYOUT
void
nsBox::AppendAttribute(const nsAutoString& aAttribute, const nsAutoString& aValue, nsAutoString& aResult)
{
   aResult.Append(aAttribute);
   aResult.AppendLiteral("='");
   aResult.Append(aValue);
   aResult.AppendLiteral("' ");
}

void
nsBox::ListBox(nsAutoString& aResult)
{
    nsAutoString name;
    GetBoxName(name);

    char addr[100];
    sprintf(addr, "[@%p] ", static_cast<void*>(this));

    aResult.AppendASCII(addr);
    aResult.Append(name);
    aResult.AppendLiteral(" ");

    nsIContent* content = GetContent();

    
    if (content) {
      nsCOMPtr<nsIDOMNode> node(do_QueryInterface(content));
      nsCOMPtr<nsIDOMNamedNodeMap> namedMap;

      node->GetAttributes(getter_AddRefs(namedMap));
      PRUint32 length;
      namedMap->GetLength(&length);

      nsCOMPtr<nsIDOMNode> attribute;
      for (PRUint32 i = 0; i < length; ++i)
      {
        namedMap->Item(i, getter_AddRefs(attribute));
        nsCOMPtr<nsIDOMAttr> attr(do_QueryInterface(attribute));
        attr->GetName(name);
        nsAutoString value;
        attr->GetValue(value);
        AppendAttribute(name, value, aResult);
      }
    }
}

NS_IMETHODIMP
nsBox::DumpBox(FILE* aFile)
{
  nsAutoString s;
  ListBox(s);
  fprintf(aFile, "%s", NS_LossyConvertUTF16toASCII(s).get());
  return NS_OK;
}

void
nsBox::PropagateDebug(nsBoxLayoutState& aState)
{
  
  if (mState & NS_STATE_DEBUG_WAS_SET) {
    if (mState & NS_STATE_SET_TO_DEBUG)
      SetDebug(aState, PR_TRUE);
    else
      SetDebug(aState, PR_FALSE);
  } else if (mState & NS_STATE_IS_ROOT) {
    SetDebug(aState, gDebug);
  }
}
#endif

#ifdef DEBUG_LAYOUT
void
nsBox::GetBoxName(nsAutoString& aName)
{
  aName.AssignLiteral("Box");
}
#endif

nsresult
nsBox::BeginLayout(nsBoxLayoutState& aState)
{
#ifdef DEBUG_LAYOUT 

  nsBoxAddIndents();
  printf("Layout: ");
  DumpBox(stdout);
  printf("\n");
  gIndent++;
#endif

  
  
  
  mState |= NS_FRAME_HAS_DIRTY_CHILDREN;

  if (GetStateBits() & NS_FRAME_IS_DIRTY)
  {
    
    
    nsIFrame* box;
    for (box = GetChildBox(); box; box = box->GetNextBox())
      box->AddStateBits(NS_FRAME_IS_DIRTY);
  }

  
  
  FrameProperties props = Properties();
  props.Delete(UsedBorderProperty());
  props.Delete(UsedPaddingProperty());
  props.Delete(UsedMarginProperty());

#ifdef DEBUG_LAYOUT
  PropagateDebug(aState);
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsBox::DoLayout(nsBoxLayoutState& aState)
{
  return NS_OK;
}

nsresult
nsBox::EndLayout(nsBoxLayoutState& aState)
{

  #ifdef DEBUG_LAYOUT
      --gIndent;
  #endif

  return SyncLayout(aState);
}

PRBool nsBox::gGotTheme = PR_FALSE;
nsITheme* nsBox::gTheme = nsnull;

nsBox::nsBox()
{
  MOZ_COUNT_CTOR(nsBox);
  
  
  if (!gGotTheme) {
    gGotTheme = PR_TRUE;
    CallGetService("@mozilla.org/chrome/chrome-native-theme;1", &gTheme);
  }
}

nsBox::~nsBox()
{
  
  
  MOZ_COUNT_DTOR(nsBox);
}

 void
nsBox::Shutdown()
{
  gGotTheme = PR_FALSE;
  NS_IF_RELEASE(gTheme);
}

NS_IMETHODIMP
nsBox::RelayoutChildAtOrdinal(nsBoxLayoutState& aState, nsIBox* aChild)
{
  return NS_OK;
}

nsresult
nsIFrame::GetClientRect(nsRect& aClientRect)
{
  aClientRect = mRect;
  aClientRect.MoveTo(0,0);

  nsMargin borderPadding;
  GetBorderAndPadding(borderPadding);

  aClientRect.Deflate(borderPadding);

  if (aClientRect.width < 0)
     aClientRect.width = 0;

  if (aClientRect.height < 0)
     aClientRect.height = 0;

 

  return NS_OK;
}

void
nsBox::SetBounds(nsBoxLayoutState& aState, const nsRect& aRect, PRBool aRemoveOverflowAreas)
{
    NS_BOX_ASSERTION(this, aRect.width >=0 && aRect.height >= 0, "SetBounds Size < 0");

    nsRect rect(mRect);

    PRUint32 flags = 0;
    GetLayoutFlags(flags);

    PRUint32 stateFlags = aState.LayoutFlags();

    flags |= stateFlags;

    if ((flags & NS_FRAME_NO_MOVE_FRAME) == NS_FRAME_NO_MOVE_FRAME)
      SetSize(nsSize(aRect.width, aRect.height));
    else
      SetRect(aRect);

    
    
    if (aRemoveOverflowAreas) {
      
      ClearOverflowRects();
    }

    if (!(flags & NS_FRAME_NO_MOVE_VIEW))
    {
      nsContainerFrame::PositionFrameView(this);
      if ((rect.x != aRect.x) || (rect.y != aRect.y))
        nsContainerFrame::PositionChildViews(this);
    }
  

   










}

void
nsBox::GetLayoutFlags(PRUint32& aFlags)
{
  aFlags = 0;
}


NS_IMETHODIMP
nsIFrame::GetBorderAndPadding(nsMargin& aBorderAndPadding)
{
  aBorderAndPadding.SizeTo(0, 0, 0, 0);
  nsresult rv = GetBorder(aBorderAndPadding);
  if (NS_FAILED(rv))
    return rv;

  nsMargin padding;
  rv = GetPadding(padding);
  if (NS_FAILED(rv))
    return rv;

  aBorderAndPadding += padding;

  return rv;
}

NS_IMETHODIMP
nsBox::GetBorder(nsMargin& aMargin)
{
  aMargin.SizeTo(0,0,0,0);
    
  const nsStyleDisplay* disp = GetStyleDisplay();
  if (disp->mAppearance && gTheme) {
    
    nsPresContext *context = PresContext();
    if (gTheme->ThemeSupportsWidget(context, this, disp->mAppearance)) {
      nsIntMargin margin(0, 0, 0, 0);
      gTheme->GetWidgetBorder(context->DeviceContext(), this,
                              disp->mAppearance, &margin);
      aMargin.top = context->DevPixelsToAppUnits(margin.top);
      aMargin.right = context->DevPixelsToAppUnits(margin.right);
      aMargin.bottom = context->DevPixelsToAppUnits(margin.bottom);
      aMargin.left = context->DevPixelsToAppUnits(margin.left);
      return NS_OK;
    }
  }

  aMargin = GetStyleBorder()->GetActualBorder();

  return NS_OK;
}

NS_IMETHODIMP
nsBox::GetPadding(nsMargin& aMargin)
{
  const nsStyleDisplay *disp = GetStyleDisplay();
  if (disp->mAppearance && gTheme) {
    
    nsPresContext *context = PresContext();
    if (gTheme->ThemeSupportsWidget(context, this, disp->mAppearance)) {
      nsIntMargin margin(0, 0, 0, 0);
      PRBool useThemePadding;

      useThemePadding = gTheme->GetWidgetPadding(context->DeviceContext(),
                                                 this, disp->mAppearance,
                                                 &margin);
      if (useThemePadding) {
        aMargin.top = context->DevPixelsToAppUnits(margin.top);
        aMargin.right = context->DevPixelsToAppUnits(margin.right);
        aMargin.bottom = context->DevPixelsToAppUnits(margin.bottom);
        aMargin.left = context->DevPixelsToAppUnits(margin.left);
        return NS_OK;
      }
    }
  }

  aMargin.SizeTo(0,0,0,0);
  GetStylePadding()->GetPadding(aMargin);

  return NS_OK;
}

NS_IMETHODIMP
nsBox::GetMargin(nsMargin& aMargin)
{
  aMargin.SizeTo(0,0,0,0);
  GetStyleMargin()->GetMargin(aMargin);

  return NS_OK;
}

void
nsBox::SizeNeedsRecalc(nsSize& aSize)
{
  aSize.width  = -1;
  aSize.height = -1;
}

void
nsBox::CoordNeedsRecalc(nscoord& aFlex)
{
  aFlex = -1;
}

PRBool
nsBox::DoesNeedRecalc(const nsSize& aSize)
{
  return (aSize.width == -1 || aSize.height == -1);
}

PRBool
nsBox::DoesNeedRecalc(nscoord aCoord)
{
  return (aCoord == -1);
}

NS_IMETHODIMP
nsBox::SetLayoutManager(nsIBoxLayout* aLayout)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBox::GetLayoutManager(nsIBoxLayout** aLayout)
{
  *aLayout = nsnull;
  return NS_OK;
}


nsSize
nsBox::GetPrefSize(nsBoxLayoutState& aState)
{
  NS_ASSERTION(aState.GetRenderingContext(), "must have rendering context");

  nsSize pref(0,0);
  DISPLAY_PREF_SIZE(this, pref);

  if (IsCollapsed(aState))
    return pref;

  AddBorderAndPadding(pref);
  PRBool widthSet, heightSet;
  nsIBox::AddCSSPrefSize(this, pref, widthSet, heightSet);

  nsSize minSize = GetMinSize(aState);
  nsSize maxSize = GetMaxSize(aState);
  return BoundsCheck(minSize, pref, maxSize);
}

nsSize
nsBox::GetMinSize(nsBoxLayoutState& aState)
{
  NS_ASSERTION(aState.GetRenderingContext(), "must have rendering context");

  nsSize min(0,0);
  DISPLAY_MIN_SIZE(this, min);

  if (IsCollapsed(aState))
    return min;

  AddBorderAndPadding(min);
  PRBool widthSet, heightSet;
  nsIBox::AddCSSMinSize(aState, this, min, widthSet, heightSet);
  return min;
}

nsSize
nsBox::GetMinSizeForScrollArea(nsBoxLayoutState& aBoxLayoutState)
{
  return nsSize(0, 0);
}

nsSize
nsBox::GetMaxSize(nsBoxLayoutState& aState)
{
  NS_ASSERTION(aState.GetRenderingContext(), "must have rendering context");

  nsSize maxSize(NS_INTRINSICSIZE, NS_INTRINSICSIZE);
  DISPLAY_MAX_SIZE(this, maxSize);

  if (IsCollapsed(aState))
    return maxSize;

  AddBorderAndPadding(maxSize);
  PRBool widthSet, heightSet;
  nsIBox::AddCSSMaxSize(this, maxSize, widthSet, heightSet);
  return maxSize;
}

nscoord
nsBox::GetFlex(nsBoxLayoutState& aState)
{
  nscoord flex = 0;

  nsIBox::AddCSSFlex(aState, this, flex);

  return flex;
}

PRUint32
nsIFrame::GetOrdinal(nsBoxLayoutState& aState)
{
  PRUint32 ordinal = DEFAULT_ORDINAL_GROUP;

  nsIContent* content = GetContent();
  if (content) {
    PRInt32 error;
    nsAutoString value;

    content->GetAttr(kNameSpaceID_None, nsGkAtoms::ordinal, value);
    if (!value.IsEmpty()) {
      ordinal = value.ToInteger(&error);
    }
    else {
      
      const nsStyleXUL* boxInfo = GetStyleXUL();
      if (boxInfo->mBoxOrdinal > 1) {
        
        ordinal = (nscoord)boxInfo->mBoxOrdinal;
      }
    }
  }

  return ordinal;
}

nscoord
nsBox::GetBoxAscent(nsBoxLayoutState& aState)
{
  if (IsCollapsed(aState))
    return 0;

  return GetPrefSize(aState).height;
}

PRBool
nsBox::IsCollapsed(nsBoxLayoutState& aState)
{
  return GetStyleVisibility()->mVisible == NS_STYLE_VISIBILITY_COLLAPSE;
}

nsresult
nsIFrame::Layout(nsBoxLayoutState& aState)
{
  NS_ASSERTION(aState.GetRenderingContext(), "must have rendering context");

  nsBox *box = static_cast<nsBox*>(this);
  DISPLAY_LAYOUT(box);

  box->BeginLayout(aState);

  box->DoLayout(aState);

  box->EndLayout(aState);

  return NS_OK;
}

PRBool
nsBox::DoesClipChildren()
{
  const nsStyleDisplay* display = GetStyleDisplay();
  NS_ASSERTION((display->mOverflowY == NS_STYLE_OVERFLOW_CLIP) ==
               (display->mOverflowX == NS_STYLE_OVERFLOW_CLIP),
               "If one overflow is clip, the other should be too");
  return display->mOverflowX == NS_STYLE_OVERFLOW_CLIP;
}

nsresult
nsBox::SyncLayout(nsBoxLayoutState& aState)
{
  







  

  if (GetStateBits() & NS_FRAME_IS_DIRTY)
     Redraw(aState);

  RemoveStateBits(NS_FRAME_HAS_DIRTY_CHILDREN | NS_FRAME_IS_DIRTY
                  | NS_FRAME_FIRST_REFLOW | NS_FRAME_IN_REFLOW);

  nsPresContext* presContext = aState.PresContext();

  PRUint32 flags = 0;
  GetLayoutFlags(flags);

  PRUint32 stateFlags = aState.LayoutFlags();

  flags |= stateFlags;

  nsRect visualOverflow;

  if (ComputesOwnOverflowArea()) {
    visualOverflow = GetVisualOverflowRect();
  }
  else {
    nsRect rect(nsPoint(0, 0), GetSize());
    nsOverflowAreas overflowAreas(rect, rect);
    if (!DoesClipChildren() && !IsCollapsed(aState)) {
      
      
      
      
      
      for (nsIFrame* kid = GetChildBox(); kid; kid = kid->GetNextBox()) {
        nsOverflowAreas kidOverflow =
          kid->GetOverflowAreas() + kid->GetPosition();
        overflowAreas.UnionWith(kidOverflow);
      }
    }

    FinishAndStoreOverflow(overflowAreas, GetSize());
    visualOverflow = overflowAreas.VisualOverflow();
  }

  nsIView* view = GetView();
  if (view) {
    
    
    nsHTMLContainerFrame::SyncFrameViewAfterReflow(
                             presContext, 
                             this,
                             view,
                             visualOverflow,
                             flags);
  } 

  return NS_OK;
}

nsresult
nsIFrame::Redraw(nsBoxLayoutState& aState,
                 const nsRect*   aDamageRect)
{
  if (aState.PaintingDisabled())
    return NS_OK;

  nsRect damageRect(0,0,0,0);
  if (aDamageRect)
    damageRect = *aDamageRect;
  else
    damageRect = GetVisualOverflowRect();

  Invalidate(damageRect);
  
  
  FrameLayerBuilder::InvalidateThebesLayersInSubtree(this);

  return NS_OK;
}

PRBool
nsIBox::AddCSSPrefSize(nsIBox* aBox, nsSize& aSize, PRBool &aWidthSet, PRBool &aHeightSet)
{
    aWidthSet = PR_FALSE;
    aHeightSet = PR_FALSE;

    
    const nsStylePosition* position = aBox->GetStylePosition();

    
    
    
    
    
    const nsStyleCoord &width = position->mWidth;
    if (width.GetUnit() == eStyleUnit_Coord) {
        aSize.width = width.GetCoordValue();
        aWidthSet = PR_TRUE;
    } else if (width.IsCalcUnit()) {
        if (!width.CalcHasPercent()) {
            
            aSize.width = nsRuleNode::ComputeComputedCalc(width, 0);
            if (aSize.width < 0)
                aSize.width = 0;
            aWidthSet = PR_TRUE;
        }
    }

    const nsStyleCoord &height = position->mHeight;
    if (height.GetUnit() == eStyleUnit_Coord) {
        aSize.height = height.GetCoordValue();
        aHeightSet = PR_TRUE;
    } else if (height.IsCalcUnit()) {
        if (!height.CalcHasPercent()) {
            
            aSize.height = nsRuleNode::ComputeComputedCalc(height, 0);
            if (aSize.height < 0)
                aSize.height = 0;
            aHeightSet = PR_TRUE;
        }
    }

    nsIContent* content = aBox->GetContent();
    
    
    
    if (content && content->IsXUL()) {
        nsAutoString value;
        PRInt32 error;

        content->GetAttr(kNameSpaceID_None, nsGkAtoms::width, value);
        if (!value.IsEmpty()) {
            value.Trim("%");

            aSize.width =
              nsPresContext::CSSPixelsToAppUnits(value.ToInteger(&error));
            aWidthSet = PR_TRUE;
        }

        content->GetAttr(kNameSpaceID_None, nsGkAtoms::height, value);
        if (!value.IsEmpty()) {
            value.Trim("%");

            aSize.height =
              nsPresContext::CSSPixelsToAppUnits(value.ToInteger(&error));
            aHeightSet = PR_TRUE;
        }
    }

    return (aWidthSet && aHeightSet);
}


PRBool
nsIBox::AddCSSMinSize(nsBoxLayoutState& aState, nsIBox* aBox, nsSize& aSize,
                      PRBool &aWidthSet, PRBool &aHeightSet)
{
    aWidthSet = PR_FALSE;
    aHeightSet = PR_FALSE;

    PRBool canOverride = PR_TRUE;

    
    const nsStyleDisplay* display = aBox->GetStyleDisplay();
    if (display->mAppearance) {
      nsITheme *theme = aState.PresContext()->GetTheme();
      if (theme && theme->ThemeSupportsWidget(aState.PresContext(), aBox, display->mAppearance)) {
        nsIntSize size;
        nsRenderingContext* rendContext = aState.GetRenderingContext();
        if (rendContext) {
          theme->GetMinimumWidgetSize(rendContext, aBox,
                                      display->mAppearance, &size, &canOverride);
          if (size.width) {
            aSize.width = aState.PresContext()->DevPixelsToAppUnits(size.width);
            aWidthSet = PR_TRUE;
          }
          if (size.height) {
            aSize.height = aState.PresContext()->DevPixelsToAppUnits(size.height);
            aHeightSet = PR_TRUE;
          }
        }
      }
    }

    
    const nsStylePosition* position = aBox->GetStylePosition();

    
    
    const nsStyleCoord &minWidth = position->mMinWidth;
    if ((minWidth.GetUnit() == eStyleUnit_Coord &&
         minWidth.GetCoordValue() != 0) ||
        (minWidth.IsCalcUnit() && !minWidth.CalcHasPercent())) {
        nscoord min = nsRuleNode::ComputeCoordPercentCalc(minWidth, 0);
        if (!aWidthSet || (min > aSize.width && canOverride)) {
           aSize.width = min;
           aWidthSet = PR_TRUE;
        }
    } else if (minWidth.GetUnit() == eStyleUnit_Percent) {
        NS_ASSERTION(minWidth.GetPercentValue() == 0.0f,
          "Non-zero percentage values not currently supported");
        aSize.width = 0;
        aWidthSet = PR_TRUE; 
                             
    }
    
    
    
    
    

    const nsStyleCoord &minHeight = position->mMinHeight;
    if ((minHeight.GetUnit() == eStyleUnit_Coord &&
         minHeight.GetCoordValue() != 0) ||
        (minHeight.IsCalcUnit() && !minHeight.CalcHasPercent())) {
        nscoord min = nsRuleNode::ComputeCoordPercentCalc(minHeight, 0);
        if (!aHeightSet || (min > aSize.height && canOverride)) {
           aSize.height = min;
           aHeightSet = PR_TRUE;
        }
    } else if (minHeight.GetUnit() == eStyleUnit_Percent) {
        NS_ASSERTION(position->mMinHeight.GetPercentValue() == 0.0f,
          "Non-zero percentage values not currently supported");
        aSize.height = 0;
        aHeightSet = PR_TRUE; 
                              
    }
    

    nsIContent* content = aBox->GetContent();
    if (content) {
        nsAutoString value;
        PRInt32 error;

        content->GetAttr(kNameSpaceID_None, nsGkAtoms::minwidth, value);
        if (!value.IsEmpty())
        {
            value.Trim("%");

            nscoord val =
              nsPresContext::CSSPixelsToAppUnits(value.ToInteger(&error));
            if (val > aSize.width)
              aSize.width = val;
            aWidthSet = PR_TRUE;
        }

        content->GetAttr(kNameSpaceID_None, nsGkAtoms::minheight, value);
        if (!value.IsEmpty())
        {
            value.Trim("%");

            nscoord val =
              nsPresContext::CSSPixelsToAppUnits(value.ToInteger(&error));
            if (val > aSize.height)
              aSize.height = val;

            aHeightSet = PR_TRUE;
        }
    }

    return (aWidthSet && aHeightSet);
}

PRBool
nsIBox::AddCSSMaxSize(nsIBox* aBox, nsSize& aSize, PRBool &aWidthSet, PRBool &aHeightSet)
{
    aWidthSet = PR_FALSE;
    aHeightSet = PR_FALSE;

    
    const nsStylePosition* position = aBox->GetStylePosition();

    
    
    
    
    
    
    const nsStyleCoord maxWidth = position->mMaxWidth;
    if (maxWidth.ConvertsToLength()) {
        aSize.width = nsRuleNode::ComputeCoordPercentCalc(maxWidth, 0);
        aWidthSet = PR_TRUE;
    }
    

    const nsStyleCoord &maxHeight = position->mMaxHeight;
    if (maxHeight.ConvertsToLength()) {
        aSize.height = nsRuleNode::ComputeCoordPercentCalc(maxHeight, 0);
        aHeightSet = PR_TRUE;
    }
    

    nsIContent* content = aBox->GetContent();
    if (content) {
        nsAutoString value;
        PRInt32 error;

        content->GetAttr(kNameSpaceID_None, nsGkAtoms::maxwidth, value);
        if (!value.IsEmpty()) {
            value.Trim("%");

            nscoord val =
              nsPresContext::CSSPixelsToAppUnits(value.ToInteger(&error));
            aSize.width = val;
            aWidthSet = PR_TRUE;
        }

        content->GetAttr(kNameSpaceID_None, nsGkAtoms::maxheight, value);
        if (!value.IsEmpty()) {
            value.Trim("%");

            nscoord val =
              nsPresContext::CSSPixelsToAppUnits(value.ToInteger(&error));
            aSize.height = val;

            aHeightSet = PR_TRUE;
        }
    }

    return (aWidthSet || aHeightSet);
}

PRBool
nsIBox::AddCSSFlex(nsBoxLayoutState& aState, nsIBox* aBox, nscoord& aFlex)
{
    PRBool flexSet = PR_FALSE;

    
    nsIContent* content = aBox->GetContent();
    if (content) {
        PRInt32 error;
        nsAutoString value;

        content->GetAttr(kNameSpaceID_None, nsGkAtoms::flex, value);
        if (!value.IsEmpty()) {
            value.Trim("%");
            aFlex = value.ToInteger(&error);
            flexSet = PR_TRUE;
        }
        else {
          
          const nsStyleXUL* boxInfo = aBox->GetStyleXUL();
          if (boxInfo->mBoxFlex > 0.0f) {
            
            aFlex = (nscoord)boxInfo->mBoxFlex;
            flexSet = PR_TRUE;
          }
        }
    }

    if (aFlex < 0)
      aFlex = 0;
    if (aFlex >= nscoord_MAX)
      aFlex = nscoord_MAX - 1;

    return flexSet;
}

void
nsBox::AddBorderAndPadding(nsSize& aSize)
{
  AddBorderAndPadding(this, aSize);
}

void
nsBox::AddBorderAndPadding(nsIBox* aBox, nsSize& aSize)
{
  nsMargin borderPadding(0,0,0,0);
  aBox->GetBorderAndPadding(borderPadding);
  AddMargin(aSize, borderPadding);
}

void
nsBox::AddMargin(nsIBox* aChild, nsSize& aSize)
{
  nsMargin margin(0,0,0,0);
  aChild->GetMargin(margin);
  AddMargin(aSize, margin);
}

void
nsBox::AddMargin(nsSize& aSize, const nsMargin& aMargin)
{
  if (aSize.width != NS_INTRINSICSIZE)
    aSize.width += aMargin.left + aMargin.right;

  if (aSize.height != NS_INTRINSICSIZE)
     aSize.height += aMargin.top + aMargin.bottom;
}

nscoord
nsBox::BoundsCheck(nscoord aMin, nscoord aPref, nscoord aMax)
{
   if (aPref > aMax)
       aPref = aMax;

   if (aPref < aMin)
       aPref = aMin;

   return aPref;
}

nsSize
nsBox::BoundsCheckMinMax(const nsSize& aMinSize, const nsSize& aMaxSize)
{
  return nsSize(NS_MAX(aMaxSize.width, aMinSize.width),
                NS_MAX(aMaxSize.height, aMinSize.height));
}

nsSize
nsBox::BoundsCheck(const nsSize& aMinSize, const nsSize& aPrefSize, const nsSize& aMaxSize)
{
  return nsSize(BoundsCheck(aMinSize.width, aPrefSize.width, aMaxSize.width),
                BoundsCheck(aMinSize.height, aPrefSize.height, aMaxSize.height));
}

#ifdef DEBUG_LAYOUT
nsresult
nsBox::SetDebug(nsBoxLayoutState& aState, PRBool aDebug)
{
    return NS_OK;
}

NS_IMETHODIMP
nsBox::GetDebugBoxAt( const nsPoint& aPoint,
                      nsIBox**     aBox)
{
  nsRect thisRect(nsPoint(0,0), GetSize());
  if (!thisRect.Contains(aPoint))
    return NS_ERROR_FAILURE;

  nsIBox* child = GetChildBox();
  nsIBox* hit = nsnull;

  *aBox = nsnull;
  while (nsnull != child) {
    nsresult rv = child->GetDebugBoxAt(aPoint - child->GetOffsetTo(this), &hit);

    if (NS_SUCCEEDED(rv) && hit) {
      *aBox = hit;
    }
    child = child->GetNextBox();
  }

  
  if (*aBox) {
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsBox::GetDebug(PRBool& aDebug)
{
  aDebug = PR_FALSE;
  return NS_OK;
}

#endif
