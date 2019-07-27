





#ifndef mozilla_dom_ScreenManagerParent_h
#define mozilla_dom_ScreenManagerParent_h

#include "mozilla/dom/PScreenManagerParent.h"
#include "nsIScreenManager.h"

namespace mozilla {
namespace dom {

class ScreenManagerParent : public PScreenManagerParent
{
 public:
  ScreenManagerParent(uint32_t* aNumberOfScreens,
                      float* aSystemDefaultScale,
                      bool* aSuccess);
  ~ScreenManagerParent() {};

  virtual bool AnswerRefresh(uint32_t* aNumberOfScreens,
                             float* aSystemDefaultScale,
                             bool* aSuccess) MOZ_OVERRIDE;

  virtual bool AnswerScreenRefresh(const uint32_t& aId,
                                   ScreenDetails* aRetVal,
                                   bool* aSuccess) MOZ_OVERRIDE;

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool AnswerGetPrimaryScreen(ScreenDetails* aRetVal,
                                      bool* aSuccess) MOZ_OVERRIDE;

  virtual bool AnswerScreenForRect(const int32_t& aLeft,
                                   const int32_t& aTop,
                                   const int32_t& aWidth,
                                   const int32_t& aHeight,
                                   ScreenDetails* aRetVal,
                                   bool* aSuccess) MOZ_OVERRIDE;

  virtual bool AnswerScreenForBrowser(PBrowserParent* aBrowser,
                                      ScreenDetails* aRetVal,
                                      bool* aSuccess);

 private:
  bool ExtractScreenDetails(nsIScreen* aScreen, ScreenDetails &aDetails);
  nsCOMPtr<nsIScreenManager> mScreenMgr;
};

} 
} 

#endif 
