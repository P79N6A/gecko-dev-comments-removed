





#ifndef AccessibleCaret_h__
#define AccessibleCaret_h__

#include "mozilla/Attributes.h"
#include "mozilla/dom/AnonymousContent.h"
#include "mozilla/dom/Element.h"
#include "nsCOMPtr.h"
#include "nsIDOMEventListener.h"
#include "nsISupportsBase.h"
#include "nsISupportsImpl.h"
#include "nsRect.h"
#include "nsRefPtr.h"
#include "nsString.h"

class nsIDocument;
class nsIFrame;
class nsIPresShell;
struct nsPoint;

namespace mozilla {













class AccessibleCaret final
{
public:
  explicit AccessibleCaret(nsIPresShell* aPresShell);
  ~AccessibleCaret();

  
  
  
  
  
  enum class Appearance : uint8_t {
    
    None,

    
    Normal,

    
    
    
    
    
    NormalNotShown,

    
    Left,

    
    Right
  };

  Appearance GetAppearance() const
  {
    return mAppearance;
  }

  void SetAppearance(Appearance aAppearance);

  
  
  bool IsLogicallyVisible() const
  {
      return mAppearance != Appearance::None;
  }

  
  bool IsVisuallyVisible() const
  {
    return (mAppearance != Appearance::None) &&
           (mAppearance != Appearance::NormalNotShown);
  }

  
  
  void SetSelectionBarEnabled(bool aEnabled);

  
  enum class PositionChangedResult : uint8_t {
    
    NotChanged,

    
    Changed,

    
    Invisible
  };
  PositionChangedResult SetPosition(nsIFrame* aFrame, int32_t aOffset);

  
  bool Intersects(const AccessibleCaret& aCaret) const;

  
  
  bool Contains(const nsPoint& aPoint) const;

  
  
  nsPoint LogicalPosition() const
  {
    return mImaginaryCaretRect.Center();
  }

  
  dom::Element* CaretElement() const
  {
    return mCaretElementHolder->GetContentNode();
  }

private:
  
  void SetCaretElementPosition(const nsRect& aRect);
  void SetSelectionBarElementPosition(const nsRect& aRect);

  
  dom::Element* CaretImageElement() const
  {
    return CaretElement()->GetFirstElementChild();
  }

  
  dom::Element* SelectionBarElement() const
  {
    return CaretElement()->GetLastElementChild();
  }

  nsIFrame* RootFrame() const
  {
    return mPresShell->GetRootFrame();
  }

  nsIFrame* CustomContentContainerFrame() const;

  
  static nsString AppearanceString(Appearance aAppearance);

  already_AddRefed<dom::Element> CreateCaretElement(nsIDocument* aDocument) const;

  
  void InjectCaretElement(nsIDocument* aDocument);

  
  void RemoveCaretElement(nsIDocument* aDocument);

  
  
  static nsPoint CaretElementPosition(const nsRect& aRect)
  {
    return aRect.TopLeft() + nsPoint(aRect.width / 2, aRect.height);
  }

  class DummyTouchListener final : public nsIDOMEventListener
  {
  public:
    NS_DECL_ISUPPORTS
    NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) override
    {
      return NS_OK;
    }

  private:
    virtual ~DummyTouchListener() {};
  };

  
  Appearance mAppearance = Appearance::None;

  bool mSelectionBarEnabled = false;

  
  
  
  
  nsIPresShell* MOZ_NON_OWNING_REF const mPresShell = nullptr;

  nsRefPtr<dom::AnonymousContent> mCaretElementHolder;

  
  nsRect mImaginaryCaretRect;

  
  
  nsRefPtr<DummyTouchListener> mDummyTouchListener{new DummyTouchListener()};

}; 

} 

#endif
