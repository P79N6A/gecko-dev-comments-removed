




#include "nsIBrowserDOMWindow.h"
#include "nsString.h"

class nsOpenURIInFrameParams MOZ_FINAL : public nsIOpenURIInFrameParams
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOPENURIINFRAMEPARAMS

  nsOpenURIInFrameParams();

private:
  ~nsOpenURIInFrameParams();
  nsString mReferrer;
  bool mIsPrivate;
};
