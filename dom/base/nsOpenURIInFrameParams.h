




#include "nsIBrowserDOMWindow.h"
#include "nsString.h"

class nsOpenURIInFrameParams final : public nsIOpenURIInFrameParams
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
