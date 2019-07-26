









































#ifndef REPRESENTATION_H_
#define REPRESENTATION_H_

#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsTArray.h"
#include "SegmentBase.h"

namespace mozilla {
namespace net {

class Representation
{
public:
  Representation() :
    mBitrate(0),
    mWidth(0),
    mHeight(0),
    mSegmentBase(nullptr)
  {
    MOZ_COUNT_CTOR(Representation);
  }
  virtual ~Representation() {
    MOZ_COUNT_DTOR(Representation);
  }

  
  int64_t const    GetBitrate() const;
  void             SetBitrate(int64_t const aBitrate);

  
  void             SetWidth(int32_t const aWidth);
  int32_t const    GetWidth() const;
  void             SetHeight(int32_t const aHeight);
  int32_t const    GetHeight() const;

  
  void             AddBaseUrl(nsAString const& aUrl);
  nsAString const& GetBaseUrl(uint32_t aIndex) const;
  bool             HasBaseUrls() const { return !mBaseUrls.IsEmpty(); }

  
  SegmentBase const* GetSegmentBase() const;
  
  void               SetSegmentBase(SegmentBase* aBase);

private:
  
  int64_t mBitrate;

  
  int32_t mWidth;
  int32_t mHeight;

  
  nsTArray<nsString> mBaseUrls;

  
  nsAutoPtr<SegmentBase> mSegmentBase;
};

}
}


#endif 
