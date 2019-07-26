








#ifndef nsIScrollFrame_h___
#define nsIScrollFrame_h___

#include "nsCoord.h"
#include "ScrollbarStyles.h"
#include "mozilla/gfx/Point.h"
#include "nsIScrollbarOwner.h"
#include "Units.h"

#define NS_DEFAULT_VERTICAL_SCROLL_DISTANCE   3
#define NS_DEFAULT_HORIZONTAL_SCROLL_DISTANCE 5

class nsBoxLayoutState;
class nsIScrollPositionListener;
class nsIFrame;
class nsPresContext;
class nsIContent;
class nsRenderingContext;
class nsIAtom;






class nsIScrollableFrame : public nsIScrollbarOwner {
public:
  typedef mozilla::CSSIntPoint CSSIntPoint;

  NS_DECL_QUERYFRAME_TARGET(nsIScrollableFrame)

  



  virtual nsIFrame* GetScrolledFrame() const = 0;

  




  virtual mozilla::ScrollbarStyles GetScrollbarStyles() const = 0;

  enum { HORIZONTAL = 0x01, VERTICAL = 0x02 };
  




  virtual uint32_t GetScrollbarVisibility() const = 0;
  




  uint32_t GetPerceivedScrollingDirections() const;
  





  virtual nsMargin GetActualScrollbarSizes() const = 0;
  




  virtual nsMargin GetDesiredScrollbarSizes(nsBoxLayoutState* aState) = 0;
  




  virtual nsMargin GetDesiredScrollbarSizes(nsPresContext* aPresContext,
                                            nsRenderingContext* aRC) = 0;
  


  virtual nscoord GetNondisappearingScrollbarWidth(nsPresContext* aPresContext,
                                                   nsRenderingContext* aRC) = 0;
  













  virtual nsRect GetScrolledRect() const = 0;
  




  virtual nsRect GetScrollPortRect() const = 0;
  




  virtual nsPoint GetScrollPosition() const = 0;
  


  virtual nsPoint GetLogicalScrollPosition() const = 0;
  







  virtual nsRect GetScrollRange() const = 0;
  



  virtual nsSize GetScrollPositionClampingScrollPortSize() const = 0;

  



  virtual nsSize GetLineScrollAmount() const = 0;
  



  virtual nsSize GetPageScrollAmount() const = 0;

  







  enum ScrollMode { INSTANT, SMOOTH, NORMAL };
  








  virtual void ScrollTo(nsPoint aScrollPosition, ScrollMode aMode,
                        const nsRect* aRange = nullptr) = 0;
  










  virtual void ScrollToCSSPixels(const CSSIntPoint& aScrollPosition) = 0;
  








  virtual void ScrollToCSSPixelsApproximate(const mozilla::CSSPoint& aScrollPosition,
                                            nsIAtom *aOrigin = nullptr) = 0;

  



  virtual CSSIntPoint GetScrollPositionCSSPixels() = 0;
  


  enum ScrollUnit { DEVICE_PIXELS, LINES, PAGES, WHOLE };
  









  virtual void ScrollBy(nsIntPoint aDelta, ScrollUnit aUnit, ScrollMode aMode,
                        nsIntPoint* aOverflow = nullptr, nsIAtom *aOrigin = nullptr) = 0;
  








  virtual void ScrollToRestoredPosition() = 0;

  



  virtual void AddScrollPositionListener(nsIScrollPositionListener* aListener) = 0;
  


  virtual void RemoveScrollPositionListener(nsIScrollPositionListener* aListener) = 0;

  



  virtual void CurPosAttributeChanged(nsIContent* aChild) = 0;

  



  NS_IMETHOD PostScrolledAreaEventForCurrentArea() = 0;

  




  virtual bool IsScrollingActive() = 0;
  



  virtual bool IsProcessingAsyncScroll() = 0;
  



  virtual void ResetScrollPositionForLayerPixelAlignment() = 0;
  


  virtual bool DidHistoryRestore() = 0;
  




  virtual void ClearDidHistoryRestore() = 0;
  



  virtual bool IsRectNearlyVisible(const nsRect& aRect) = 0;
  



  virtual nsIAtom* OriginOfLastScroll() = 0;
  



  virtual uint32_t CurrentScrollGeneration() = 0;
  




  virtual void ResetOriginIfScrollAtGeneration(uint32_t aGeneration) = 0;
  



  virtual bool WantAsyncScroll() const = 0;
};

#endif
