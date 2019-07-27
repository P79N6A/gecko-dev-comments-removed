





#include "AccessibleCaret.h"

#include "AccessibleCaretLogger.h"
#include "mozilla/Preferences.h"
#include "nsCanvasFrame.h"
#include "nsCaret.h"
#include "nsDOMTokenList.h"
#include "nsIFrame.h"

namespace mozilla {
using namespace dom;

#undef AC_LOG
#define AC_LOG(message, ...)                                                   \
  AC_LOG_BASE("AccessibleCaret (%p): " message, this, ##__VA_ARGS__);

#undef AC_LOGV
#define AC_LOGV(message, ...)                                                  \
  AC_LOGV_BASE("AccessibleCaret (%p): " message, this, ##__VA_ARGS__);

NS_IMPL_ISUPPORTS(AccessibleCaret::DummyTouchListener, nsIDOMEventListener)




AccessibleCaret::AccessibleCaret(nsIPresShell* aPresShell)
  : mPresShell(aPresShell)
{
  
  MOZ_ASSERT(mPresShell);
  MOZ_ASSERT(RootFrame());
  MOZ_ASSERT(mPresShell->GetDocument());
  MOZ_ASSERT(mPresShell->GetCanvasFrame());
  MOZ_ASSERT(mPresShell->GetCanvasFrame()->GetCustomContentContainer());

  InjectCaretElement(mPresShell->GetDocument());
}

AccessibleCaret::~AccessibleCaret()
{
  RemoveCaretElement(mPresShell->GetDocument());
}

void
AccessibleCaret::SetAppearance(Appearance aAppearance)
{
  if (mAppearance == aAppearance) {
    return;
  }

  ErrorResult rv;
  CaretElement()->ClassList()->Remove(AppearanceString(mAppearance), rv);
  MOZ_ASSERT(!rv.Failed(), "Remove old appearance failed!");

  CaretElement()->ClassList()->Add(AppearanceString(aAppearance), rv);
  MOZ_ASSERT(!rv.Failed(), "Add new appearance failed!");

  mAppearance = aAppearance;

  
  if (mAppearance == Appearance::None) {
    mImaginaryCaretRect = nsRect();
  }
}

void
AccessibleCaret::SetSelectionBarEnabled(bool aEnabled)
{
  if (mSelectionBarEnabled == aEnabled) {
    return;
  }

  AC_LOG("%s, enabled %d", __FUNCTION__, aEnabled);

  ErrorResult rv;
  CaretElement()->ClassList()->Toggle(NS_LITERAL_STRING("no-bar"),
                                      Optional<bool>(!aEnabled), rv);
  MOZ_ASSERT(!rv.Failed());

  mSelectionBarEnabled = aEnabled;
}

 nsString
AccessibleCaret::AppearanceString(Appearance aAppearance)
{
  nsAutoString string;
  switch (aAppearance) {
  case Appearance::None:
  case Appearance::NormalNotShown:
    string = NS_LITERAL_STRING("none");
    break;
  case Appearance::Normal:
    string = NS_LITERAL_STRING("normal");
    break;
  case Appearance::Right:
    string = NS_LITERAL_STRING("right");
    break;
  case Appearance::Left:
    string = NS_LITERAL_STRING("left");
    break;
  }
  return string;
}

bool
AccessibleCaret::Intersects(const AccessibleCaret& aCaret) const
{
  MOZ_ASSERT(mPresShell == aCaret.mPresShell);

  if (!IsVisuallyVisible() || !aCaret.IsVisuallyVisible()) {
    return false;
  }

  nsRect rect = nsLayoutUtils::GetRectRelativeToFrame(CaretElement(), RootFrame());
  nsRect rhsRect = nsLayoutUtils::GetRectRelativeToFrame(aCaret.CaretElement(), RootFrame());
  return rect.Intersects(rhsRect);
}

bool
AccessibleCaret::Contains(const nsPoint& aPoint) const
{
  if (!IsVisuallyVisible()) {
    return false;
  }

  nsRect rect =
    nsLayoutUtils::GetRectRelativeToFrame(CaretImageElement(), RootFrame());

  return rect.Contains(aPoint);
}

void
AccessibleCaret::InjectCaretElement(nsIDocument* aDocument)
{
  ErrorResult rv;
  nsCOMPtr<Element> element = CreateCaretElement(aDocument);
  mCaretElementHolder = aDocument->InsertAnonymousContent(*element, rv);

  MOZ_ASSERT(!rv.Failed(), "Insert anonymous content should not fail!");
  MOZ_ASSERT(mCaretElementHolder.get(), "We must have anonymous content!");

  
  
  
  CaretElement()->AddEventListener(NS_LITERAL_STRING("touchstart"),
                                   mDummyTouchListener, false);
}

already_AddRefed<Element>
AccessibleCaret::CreateCaretElement(nsIDocument* aDocument) const
{
  
  
  
  

  ErrorResult rv;
  nsCOMPtr<Element> parent = aDocument->CreateHTMLElement(nsGkAtoms::div);
  parent->ClassList()->Add(NS_LITERAL_STRING("moz-accessiblecaret"), rv);
  parent->ClassList()->Add(NS_LITERAL_STRING("none"), rv);
  parent->ClassList()->Add(NS_LITERAL_STRING("no-bar"), rv);

  nsCOMPtr<Element> image = aDocument->CreateHTMLElement(nsGkAtoms::div);
  image->ClassList()->Add(NS_LITERAL_STRING("image"), rv);
  parent->AppendChildTo(image, false);

  nsCOMPtr<Element> bar = aDocument->CreateHTMLElement(nsGkAtoms::div);
  bar->ClassList()->Add(NS_LITERAL_STRING("bar"), rv);
  parent->AppendChildTo(bar, false);

  return parent.forget();
}

void
AccessibleCaret::RemoveCaretElement(nsIDocument* aDocument)
{
  CaretElement()->RemoveEventListener(NS_LITERAL_STRING("touchstart"),
                                      mDummyTouchListener, false);

  ErrorResult rv;
  aDocument->RemoveAnonymousContent(*mCaretElementHolder, rv);
  
}

AccessibleCaret::PositionChangedResult
AccessibleCaret::SetPosition(nsIFrame* aFrame, int32_t aOffset)
{
  if (!CustomContentContainerFrame()) {
    return PositionChangedResult::NotChanged;
  }

  nsRect imaginaryCaretRectInFrame =
    nsCaret::GetGeometryForFrame(aFrame, aOffset, nullptr);

  imaginaryCaretRectInFrame =
    nsLayoutUtils::ClampRectToScrollFrames(aFrame, imaginaryCaretRectInFrame);

  if (imaginaryCaretRectInFrame.IsEmpty()) {
    
    return PositionChangedResult::Invisible;
  }

  nsRect imaginaryCaretRect = imaginaryCaretRectInFrame;
  nsLayoutUtils::TransformRect(aFrame, RootFrame(), imaginaryCaretRect);

  if (imaginaryCaretRect.IsEqualEdges(mImaginaryCaretRect)) {
    return PositionChangedResult::NotChanged;
  }

  mImaginaryCaretRect = imaginaryCaretRect;

  
  
  nsRect imaginaryCaretRectInContainerFrame = imaginaryCaretRectInFrame;
  nsLayoutUtils::TransformRect(aFrame, CustomContentContainerFrame(),
                               imaginaryCaretRectInContainerFrame);
  SetCaretElementPosition(imaginaryCaretRectInContainerFrame);
  SetSelectionBarElementPosition(imaginaryCaretRectInContainerFrame);

  return PositionChangedResult::Changed;
}

nsIFrame*
AccessibleCaret::CustomContentContainerFrame() const
{
  nsCanvasFrame* canvasFrame = mPresShell->GetCanvasFrame();
  Element* container = canvasFrame->GetCustomContentContainer();
  nsIFrame* containerFrame = container->GetPrimaryFrame();
  return containerFrame;
}

void
AccessibleCaret::SetCaretElementPosition(const nsRect& aRect)
{
  nsPoint position = CaretElementPosition(aRect);
  nsAutoString styleStr;
  styleStr.AppendPrintf("left: %dpx; top: %dpx;",
                        nsPresContext::AppUnitsToIntCSSPixels(position.x),
                        nsPresContext::AppUnitsToIntCSSPixels(position.y));

  ErrorResult rv;
  CaretElement()->SetAttribute(NS_LITERAL_STRING("style"), styleStr, rv);
  MOZ_ASSERT(!rv.Failed());

  AC_LOG("Set caret style: %s", NS_ConvertUTF16toUTF8(styleStr).get());
}

void
AccessibleCaret::SetSelectionBarElementPosition(const nsRect& aRect)
{
  int32_t height = nsPresContext::AppUnitsToIntCSSPixels(aRect.height);
  nsAutoString barStyleStr;
  barStyleStr.AppendPrintf("margin-top: -%dpx; height: %dpx;",
                           height, height);

  ErrorResult rv;
  SelectionBarElement()->SetAttribute(NS_LITERAL_STRING("style"), barStyleStr, rv);
  MOZ_ASSERT(!rv.Failed());

  AC_LOG("Set bar style: %s", NS_ConvertUTF16toUTF8(barStyleStr).get());
}

} 
