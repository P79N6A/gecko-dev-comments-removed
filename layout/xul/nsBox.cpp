




#include "nsBoxLayoutState.h"
#include "nsBox.h"
#include "nsBoxFrame.h"
#include "nsPresContext.h"
#include "nsCOMPtr.h"
#include "nsIContent.h"
#include "nsContainerFrame.h"
#include "nsNameSpaceManager.h"
#include "nsGkAtoms.h"
#include "nsIDOMNode.h"
#include "nsIDOMMozNamedAttrMap.h"
#include "nsIDOMAttr.h"
#include "nsITheme.h"
#include "nsIServiceManager.h"
#include "nsBoxLayout.h"
#include "FrameLayerBuilder.h"
#include <algorithm>

using namespace mozilla;

#ifdef DEBUG_LAYOUT
int32_t gIndent = 0;
#endif

#ifdef DEBUG_LAYOUT
void
nsBoxAddIndents()
{
    for(int32_t i=0; i < gIndent; i++)
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
    aResult.Append(' ');

    nsIContent* content = GetContent();

    
    if (content) {
      nsCOMPtr<nsIDOMNode> node(do_QueryInterface(content));
      nsCOMPtr<nsIDOMMozNamedAttrMap> namedMap;

      node->GetAttributes(getter_AddRefs(namedMap));
      uint32_t length;
      namedMap->GetLength(&length);

      nsCOMPtr<nsIDOMAttr> attribute;
      for (uint32_t i = 0; i < length; ++i)
      {
        namedMap->Item(i, getter_AddRefs(attribute));
        attribute->GetName(name);
        nsAutoString value;
        attribute->GetValue(value);
        AppendAttribute(name, value, aResult);
      }
    }
}

nsresult
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
      SetDebug(aState, true);
    else
      SetDebug(aState, false);
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
    for (box = GetChildBox(this); box; box = GetNextBox(box))
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

bool nsBox::gGotTheme = false;
nsITheme* nsBox::gTheme = nullptr;

nsBox::nsBox()
{
  MOZ_COUNT_CTOR(nsBox);
  
  
  if (!gGotTheme) {
    gGotTheme = true;
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
  gGotTheme = false;
  NS_IF_RELEASE(gTheme);
}

nsresult
nsBox::RelayoutChildAtOrdinal(nsBoxLayoutState& aState, nsIFrame* aChild)
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
nsBox::SetBounds(nsBoxLayoutState& aState, const nsRect& aRect, bool aRemoveOverflowAreas)
{
    NS_BOX_ASSERTION(this, aRect.width >=0 && aRect.height >= 0, "SetBounds Size < 0");

    nsRect rect(mRect);

    uint32_t flags = 0;
    GetLayoutFlags(flags);

    uint32_t stateFlags = aState.LayoutFlags();

    flags |= stateFlags;

    if ((flags & NS_FRAME_NO_MOVE_FRAME) == NS_FRAME_NO_MOVE_FRAME)
      SetSize(aRect.Size());
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
nsBox::GetLayoutFlags(uint32_t& aFlags)
{
  aFlags = 0;
}


nsresult
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

nsresult
nsBox::GetBorder(nsMargin& aMargin)
{
  aMargin.SizeTo(0,0,0,0);
    
  const nsStyleDisplay* disp = StyleDisplay();
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

  aMargin = StyleBorder()->GetComputedBorder();

  return NS_OK;
}

nsresult
nsBox::GetPadding(nsMargin& aMargin)
{
  const nsStyleDisplay *disp = StyleDisplay();
  if (disp->mAppearance && gTheme) {
    
    nsPresContext *context = PresContext();
    if (gTheme->ThemeSupportsWidget(context, this, disp->mAppearance)) {
      nsIntMargin margin(0, 0, 0, 0);
      bool useThemePadding;

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
  StylePadding()->GetPadding(aMargin);

  return NS_OK;
}

nsresult
nsBox::GetMargin(nsMargin& aMargin)
{
  aMargin.SizeTo(0,0,0,0);
  StyleMargin()->GetMargin(aMargin);

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

bool
nsBox::DoesNeedRecalc(const nsSize& aSize)
{
  return (aSize.width == -1 || aSize.height == -1);
}

bool
nsBox::DoesNeedRecalc(nscoord aCoord)
{
  return (aCoord == -1);
}

nsSize
nsBox::GetPrefSize(nsBoxLayoutState& aState)
{
  NS_ASSERTION(aState.GetRenderingContext(), "must have rendering context");

  nsSize pref(0,0);
  DISPLAY_PREF_SIZE(this, pref);

  if (IsCollapsed())
    return pref;

  AddBorderAndPadding(pref);
  bool widthSet, heightSet;
  nsIFrame::AddCSSPrefSize(this, pref, widthSet, heightSet);

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

  if (IsCollapsed())
    return min;

  AddBorderAndPadding(min);
  bool widthSet, heightSet;
  nsIFrame::AddCSSMinSize(aState, this, min, widthSet, heightSet);
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

  if (IsCollapsed())
    return maxSize;

  AddBorderAndPadding(maxSize);
  bool widthSet, heightSet;
  nsIFrame::AddCSSMaxSize(this, maxSize, widthSet, heightSet);
  return maxSize;
}

nscoord
nsBox::GetFlex(nsBoxLayoutState& aState)
{
  nscoord flex = 0;

  nsIFrame::AddCSSFlex(aState, this, flex);

  return flex;
}

uint32_t
nsIFrame::GetOrdinal()
{
  uint32_t ordinal = StyleXUL()->mBoxOrdinal;

  
  nsIContent* content = GetContent();
  if (content && content->IsXUL()) {
    nsresult error;
    nsAutoString value;

    content->GetAttr(kNameSpaceID_None, nsGkAtoms::ordinal, value);
    if (!value.IsEmpty()) {
      ordinal = value.ToInteger(&error);
    }
  }

  return ordinal;
}

nscoord
nsBox::GetBoxAscent(nsBoxLayoutState& aState)
{
  if (IsCollapsed())
    return 0;

  return GetPrefSize(aState).height;
}

bool
nsBox::IsCollapsed()
{
  return StyleVisibility()->mVisible == NS_STYLE_VISIBILITY_COLLAPSE;
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

bool
nsBox::DoesClipChildren()
{
  const nsStyleDisplay* display = StyleDisplay();
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

  uint32_t flags = 0;
  GetLayoutFlags(flags);

  uint32_t stateFlags = aState.LayoutFlags();

  flags |= stateFlags;

  nsRect visualOverflow;

  if (ComputesOwnOverflowArea()) {
    visualOverflow = GetVisualOverflowRect();
  }
  else {
    nsRect rect(nsPoint(0, 0), GetSize());
    nsOverflowAreas overflowAreas(rect, rect);
    if (!DoesClipChildren() && !IsCollapsed()) {
      
      
      
      
      
      nsLayoutUtils::UnionChildOverflow(this, overflowAreas);
    }

    FinishAndStoreOverflow(overflowAreas, GetSize());
    visualOverflow = overflowAreas.VisualOverflow();
  }

  nsView* view = GetView();
  if (view) {
    
    
    nsContainerFrame::SyncFrameViewAfterReflow(presContext, this, view,
                                               visualOverflow, flags);
  } 

  return NS_OK;
}

nsresult
nsIFrame::Redraw(nsBoxLayoutState& aState)
{
  if (aState.PaintingDisabled())
    return NS_OK;

  
  
  InvalidateFrameSubtree();

  return NS_OK;
}

bool
nsIFrame::AddCSSPrefSize(nsIFrame* aBox, nsSize& aSize, bool &aWidthSet, bool &aHeightSet)
{
    aWidthSet = false;
    aHeightSet = false;

    
    const nsStylePosition* position = aBox->StylePosition();

    
    
    
    
    
    const nsStyleCoord &width = position->mWidth;
    if (width.GetUnit() == eStyleUnit_Coord) {
        aSize.width = width.GetCoordValue();
        aWidthSet = true;
    } else if (width.IsCalcUnit()) {
        if (!width.CalcHasPercent()) {
            
            aSize.width = nsRuleNode::ComputeComputedCalc(width, 0);
            if (aSize.width < 0)
                aSize.width = 0;
            aWidthSet = true;
        }
    }

    const nsStyleCoord &height = position->mHeight;
    if (height.GetUnit() == eStyleUnit_Coord) {
        aSize.height = height.GetCoordValue();
        aHeightSet = true;
    } else if (height.IsCalcUnit()) {
        if (!height.CalcHasPercent()) {
            
            aSize.height = nsRuleNode::ComputeComputedCalc(height, 0);
            if (aSize.height < 0)
                aSize.height = 0;
            aHeightSet = true;
        }
    }

    nsIContent* content = aBox->GetContent();
    
    
    
    if (content && content->IsXUL()) {
        nsAutoString value;
        nsresult error;

        content->GetAttr(kNameSpaceID_None, nsGkAtoms::width, value);
        if (!value.IsEmpty()) {
            value.Trim("%");

            aSize.width =
              nsPresContext::CSSPixelsToAppUnits(value.ToInteger(&error));
            aWidthSet = true;
        }

        content->GetAttr(kNameSpaceID_None, nsGkAtoms::height, value);
        if (!value.IsEmpty()) {
            value.Trim("%");

            aSize.height =
              nsPresContext::CSSPixelsToAppUnits(value.ToInteger(&error));
            aHeightSet = true;
        }
    }

    return (aWidthSet && aHeightSet);
}


bool
nsIFrame::AddCSSMinSize(nsBoxLayoutState& aState, nsIFrame* aBox, nsSize& aSize,
                      bool &aWidthSet, bool &aHeightSet)
{
    aWidthSet = false;
    aHeightSet = false;

    bool canOverride = true;

    
    const nsStyleDisplay* display = aBox->StyleDisplay();
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
            aWidthSet = true;
          }
          if (size.height) {
            aSize.height = aState.PresContext()->DevPixelsToAppUnits(size.height);
            aHeightSet = true;
          }
        }
      }
    }

    
    const nsStylePosition* position = aBox->StylePosition();

    
    
    const nsStyleCoord &minWidth = position->mMinWidth;
    if ((minWidth.GetUnit() == eStyleUnit_Coord &&
         minWidth.GetCoordValue() != 0) ||
        (minWidth.IsCalcUnit() && !minWidth.CalcHasPercent())) {
        nscoord min = nsRuleNode::ComputeCoordPercentCalc(minWidth, 0);
        if (!aWidthSet || (min > aSize.width && canOverride)) {
           aSize.width = min;
           aWidthSet = true;
        }
    } else if (minWidth.GetUnit() == eStyleUnit_Percent) {
        NS_ASSERTION(minWidth.GetPercentValue() == 0.0f,
          "Non-zero percentage values not currently supported");
        aSize.width = 0;
        aWidthSet = true; 
                             
    }
    
    
    
    
    

    const nsStyleCoord &minHeight = position->mMinHeight;
    if ((minHeight.GetUnit() == eStyleUnit_Coord &&
         minHeight.GetCoordValue() != 0) ||
        (minHeight.IsCalcUnit() && !minHeight.CalcHasPercent())) {
        nscoord min = nsRuleNode::ComputeCoordPercentCalc(minHeight, 0);
        if (!aHeightSet || (min > aSize.height && canOverride)) {
           aSize.height = min;
           aHeightSet = true;
        }
    } else if (minHeight.GetUnit() == eStyleUnit_Percent) {
        NS_ASSERTION(position->mMinHeight.GetPercentValue() == 0.0f,
          "Non-zero percentage values not currently supported");
        aSize.height = 0;
        aHeightSet = true; 
                              
    }
    

    nsIContent* content = aBox->GetContent();
    if (content && content->IsXUL()) {
        nsAutoString value;
        nsresult error;

        content->GetAttr(kNameSpaceID_None, nsGkAtoms::minwidth, value);
        if (!value.IsEmpty())
        {
            value.Trim("%");

            nscoord val =
              nsPresContext::CSSPixelsToAppUnits(value.ToInteger(&error));
            if (val > aSize.width)
              aSize.width = val;
            aWidthSet = true;
        }

        content->GetAttr(kNameSpaceID_None, nsGkAtoms::minheight, value);
        if (!value.IsEmpty())
        {
            value.Trim("%");

            nscoord val =
              nsPresContext::CSSPixelsToAppUnits(value.ToInteger(&error));
            if (val > aSize.height)
              aSize.height = val;

            aHeightSet = true;
        }
    }

    return (aWidthSet && aHeightSet);
}

bool
nsIFrame::AddCSSMaxSize(nsIFrame* aBox, nsSize& aSize, bool &aWidthSet, bool &aHeightSet)
{
    aWidthSet = false;
    aHeightSet = false;

    
    const nsStylePosition* position = aBox->StylePosition();

    
    
    
    
    
    
    const nsStyleCoord maxWidth = position->mMaxWidth;
    if (maxWidth.ConvertsToLength()) {
        aSize.width = nsRuleNode::ComputeCoordPercentCalc(maxWidth, 0);
        aWidthSet = true;
    }
    

    const nsStyleCoord &maxHeight = position->mMaxHeight;
    if (maxHeight.ConvertsToLength()) {
        aSize.height = nsRuleNode::ComputeCoordPercentCalc(maxHeight, 0);
        aHeightSet = true;
    }
    

    nsIContent* content = aBox->GetContent();
    if (content && content->IsXUL()) {
        nsAutoString value;
        nsresult error;

        content->GetAttr(kNameSpaceID_None, nsGkAtoms::maxwidth, value);
        if (!value.IsEmpty()) {
            value.Trim("%");

            nscoord val =
              nsPresContext::CSSPixelsToAppUnits(value.ToInteger(&error));
            aSize.width = val;
            aWidthSet = true;
        }

        content->GetAttr(kNameSpaceID_None, nsGkAtoms::maxheight, value);
        if (!value.IsEmpty()) {
            value.Trim("%");

            nscoord val =
              nsPresContext::CSSPixelsToAppUnits(value.ToInteger(&error));
            aSize.height = val;

            aHeightSet = true;
        }
    }

    return (aWidthSet || aHeightSet);
}

bool
nsIFrame::AddCSSFlex(nsBoxLayoutState& aState, nsIFrame* aBox, nscoord& aFlex)
{
    bool flexSet = false;

    
    aFlex = aBox->StyleXUL()->mBoxFlex;

    
    nsIContent* content = aBox->GetContent();
    if (content && content->IsXUL()) {
        nsresult error;
        nsAutoString value;

        content->GetAttr(kNameSpaceID_None, nsGkAtoms::flex, value);
        if (!value.IsEmpty()) {
            value.Trim("%");
            aFlex = value.ToInteger(&error);
            flexSet = true;
        }
    }

    if (aFlex < 0)
      aFlex = 0;
    if (aFlex >= nscoord_MAX)
      aFlex = nscoord_MAX - 1;

    return flexSet || aFlex > 0;
}

void
nsBox::AddBorderAndPadding(nsSize& aSize)
{
  AddBorderAndPadding(this, aSize);
}

void
nsBox::AddBorderAndPadding(nsIFrame* aBox, nsSize& aSize)
{
  nsMargin borderPadding(0,0,0,0);
  aBox->GetBorderAndPadding(borderPadding);
  AddMargin(aSize, borderPadding);
}

void
nsBox::AddMargin(nsIFrame* aChild, nsSize& aSize)
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
  return nsSize(std::max(aMaxSize.width, aMinSize.width),
                std::max(aMaxSize.height, aMinSize.height));
}

nsSize
nsBox::BoundsCheck(const nsSize& aMinSize, const nsSize& aPrefSize, const nsSize& aMaxSize)
{
  return nsSize(BoundsCheck(aMinSize.width, aPrefSize.width, aMaxSize.width),
                BoundsCheck(aMinSize.height, aPrefSize.height, aMaxSize.height));
}

 nsIFrame*
nsBox::GetChildBox(const nsIFrame* aFrame)
{
  
  
  return aFrame->IsBoxFrame() ? aFrame->GetFirstPrincipalChild() : nullptr;
}

 nsIFrame*
nsBox::GetNextBox(const nsIFrame* aFrame)
{
  return aFrame->GetParent() &&
    aFrame->GetParent()->IsBoxFrame() ? aFrame->GetNextSibling() : nullptr;
}

 nsIFrame*
nsBox::GetParentBox(const nsIFrame* aFrame)
{
  return aFrame->GetParent() &&
    aFrame->GetParent()->IsBoxFrame() ? aFrame->GetParent() : nullptr;
}

#ifdef DEBUG_LAYOUT
nsresult
nsBox::SetDebug(nsBoxLayoutState& aState, bool aDebug)
{
    return NS_OK;
}

NS_IMETHODIMP
nsBox::GetDebugBoxAt( const nsPoint& aPoint,
                      nsIFrame**     aBox)
{
  nsRect thisRect(nsPoint(0,0), GetSize());
  if (!thisRect.Contains(aPoint))
    return NS_ERROR_FAILURE;

  nsIFrame* child = nsBox::GetChildBox(this);
  nsIFrame* hit = nullptr;

  *aBox = nullptr;
  while (nullptr != child) {
    nsresult rv = child->GetDebugBoxAt(aPoint - child->GetOffsetTo(this), &hit);

    if (NS_SUCCEEDED(rv) && hit) {
      *aBox = hit;
    }
    child = GetNextBox(child);
  }

  
  if (*aBox) {
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}


nsresult
nsBox::GetDebug(bool& aDebug)
{
  aDebug = false;
  return NS_OK;
}

#endif
