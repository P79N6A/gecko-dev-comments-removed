









#ifndef mock_Link_h__
#define mock_Link_h__

#include "mozilla/dom/Link.h"

class mock_Link : public mozilla::dom::Link
{
public:
  NS_DECL_ISUPPORTS

  mock_Link(void (*aHandlerFunction)(nsLinkState),
            bool aRunNextTest = true)
  : mozilla::dom::Link(nullptr)
  , mHandler(aHandlerFunction)
  , mRunNextTest(aRunNextTest)
  {
    
    
    
    
    
    mDeathGrip = this;
  }

  virtual void SetLinkState(nsLinkState aState)
  {
    
    mHandler(aState);

    
    mDeathGrip = 0;
  }

  virtual size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const
  {
    return 0;   
  }

  ~mock_Link() {
    
    if (mRunNextTest) {
      run_next_test();
    }
  }

private:
  void (*mHandler)(nsLinkState);
  bool mRunNextTest;
  nsRefPtr<Link> mDeathGrip;
};

NS_IMPL_ISUPPORTS1(
  mock_Link,
  mozilla::dom::Link
)




namespace mozilla {
namespace dom {

Link::Link(Element* aElement)
: mElement(aElement)
, mLinkState(eLinkState_NotLink)
, mRegistered(false)
{
}

Link::~Link()
{
}

bool
Link::ElementHasHref() const
{
  NS_NOTREACHED("Unexpected call to Link::ElementHasHref");
  return false; 
}

nsLinkState
Link::GetLinkState() const
{
  NS_NOTREACHED("Unexpected call to Link::GetLinkState");
  return eLinkState_NotLink; 
}

void
Link::SetLinkState(nsLinkState aState)
{
  NS_NOTREACHED("Unexpected call to Link::SetLinkState");
}

void
Link::ResetLinkState(bool aNotify, bool aHasHref)
{
  NS_NOTREACHED("Unexpected call to Link::ResetLinkState");
}

already_AddRefed<nsIURI>
Link::GetURI() const 
{
  NS_NOTREACHED("Unexpected call to Link::GetURI");
  return nullptr; 
}

size_t
Link::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
  NS_NOTREACHED("Unexpected call to Link::SizeOfExcludingThis");
  return 0;
}

} 
} 

#endif 
