










































#ifndef mozilla_dom_Link_h__
#define mozilla_dom_Link_h__

#include "nsIContent.h"

namespace mozilla {
namespace dom {

class Link : public nsISupports
{
public:
  static const nsLinkState defaultState = eLinkState_Unknown;
  Link();
  virtual nsLinkState GetLinkState() const;
  virtual void SetLinkState(nsLinkState aState);

protected:
  


  virtual void ResetLinkState();

  nsLinkState mLinkState;
};

} 
} 

#endif 
