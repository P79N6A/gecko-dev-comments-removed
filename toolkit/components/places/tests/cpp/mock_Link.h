










































#ifndef mock_Link_h__
#define mock_Link_h__

#include "mozilla/dom/Link.h"

class mock_Link : public mozilla::dom::Link
{
public:
  NS_DECL_ISUPPORTS

  mock_Link(void (*aHandlerFunction)(nsLinkState),
            bool aRunNextTest = true)
  : mozilla::dom::Link(nsnull)
  , mHandler(aHandlerFunction)
  , mRunNextTest(aRunNextTest)
  {
  }

  virtual void SetLinkState(nsLinkState aState)
  {
    
    mHandler(aState);

    
    if (mRunNextTest) {
      run_next_test();
    }

    
    NS_RELEASE_THIS();
  }

private:
  void (*mHandler)(nsLinkState);
  bool mRunNextTest;
};

NS_IMPL_ISUPPORTS1(
  mock_Link,
  mozilla::dom::Link
)




namespace mozilla {
namespace dom {

Link::Link(Element* aElement)
: mLinkState(mozilla::dom::Link::defaultState)
, mRegistered(false)
, mElement(aElement)
{
}

Link::~Link()
{
}

nsLinkState
Link::GetLinkState() const
{
  NS_NOTREACHED("Unexpected call to Link::GetLinkState");
  return eLinkState_Unknown; 
}

void
Link::SetLinkState(nsLinkState aState)
{
  NS_NOTREACHED("Unexpected call to Link::SetLinkState");
}

void
Link::ResetLinkState(bool aNotify)
{
  NS_NOTREACHED("Unexpected call to Link::ResetLinkState");
}

already_AddRefed<nsIURI>
Link::GetURI() const 
{
  NS_NOTREACHED("Unexpected call to Link::GetURI");
  return nsnull; 
}

} 
} 

#endif 
