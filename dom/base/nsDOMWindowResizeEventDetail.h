



#include "nsIDOMDOMWindowResizeEventDetail.h"
#include "nsSize.h"

class nsDOMWindowResizeEventDetail : public nsIDOMDOMWindowResizeEventDetail
{
public:
  nsDOMWindowResizeEventDetail(const nsIntSize& aSize)
    : mSize(aSize)
  {}

  virtual ~nsDOMWindowResizeEventDetail() {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMDOMWINDOWRESIZEEVENTDETAIL

private:
  const nsIntSize mSize;
};
