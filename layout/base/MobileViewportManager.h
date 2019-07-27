




#ifndef MobileViewportManager_h_
#define MobileViewportManager_h_

#include "mozilla/Maybe.h"
#include "nsIDOMEventListener.h"
#include "nsIDocument.h"
#include "nsIObserver.h"
#include "Units.h"

class nsIDOMEventTarget;
class nsIDocument;
class nsIPresShell;

class MobileViewportManager final : public nsIDOMEventListener
                                  , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER
  NS_DECL_NSIOBSERVER

  MobileViewportManager(nsIPresShell* aPresShell,
                        nsIDocument* aDocument);
  void Destroy();

  

  void RequestReflow();

private:
  ~MobileViewportManager();

  

  void RefreshViewportSize(bool aForceAdjustResolution);

  
  mozilla::CSSToScreenScale UpdateResolution(const nsViewportInfo& aViewportInfo,
                                             const mozilla::ScreenIntSize& aDisplaySize,
                                             const mozilla::CSSSize& aViewport,
                                             const mozilla::Maybe<float>& aDisplayWidthChangeRatio);
  
  void UpdateSPCSPS(const mozilla::ScreenIntSize& aDisplaySize,
                    const mozilla::CSSToScreenScale& aZoom);
  
  void UpdateDisplayPortMargins();

  nsCOMPtr<nsIDocument> mDocument;
  nsIPresShell* MOZ_NON_OWNING_REF mPresShell; 
  nsCOMPtr<nsIDOMEventTarget> mEventTarget;
  bool mIsFirstPaint;
  mozilla::LayoutDeviceIntSize mDisplaySize;
  mozilla::CSSSize mMobileViewportSize;
};

#endif

