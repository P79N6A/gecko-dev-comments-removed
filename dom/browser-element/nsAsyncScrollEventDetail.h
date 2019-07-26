



#include "nsIAsyncScrollEventDetail.h"





class nsAsyncScrollEventDetail : public nsIAsyncScrollEventDetail
{
public:
  nsAsyncScrollEventDetail(const float left, const float top,
                           const float width, const float height,
                           const float contentWidth, const float contentHeigh)
    : mTop(top)
    , mLeft(left)
    , mWidth(width)
    , mHeight(height)
    , mScrollWidth(contentWidth)
    , mScrollHeight(contentHeigh)
  {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSIASYNCSCROLLEVENTDETAIL

private:
  virtual ~nsAsyncScrollEventDetail() {}
  const float mTop;
  const float mLeft;
  const float mWidth;
  const float mHeight;
  const float mScrollWidth;
  const float mScrollHeight;
};
