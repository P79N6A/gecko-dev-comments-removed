









































#ifndef MPD_H_
#define MPD_H_

#include "nsTArray.h"
#include "nsString.h"
#include "Period.h"

namespace mozilla {
namespace net {

class MPD
{
public:
  MPD()
  {
    MOZ_COUNT_CTOR(MPD);
  }
  virtual ~MPD() {
    MOZ_COUNT_DTOR(MPD);
  }

  
  
  
  
  void           AddPeriod(Period* aPeriod);
  Period const * GetPeriod(uint32_t aIndex) const;
  uint32_t const GetNumPeriods() const;

  
  
  
  void             AddBaseUrl(nsAString const& aUrl);
  nsAString const& GetBaseUrl(uint32_t aIndex) const;
  bool             HasBaseUrls() { return !mBaseUrls.IsEmpty(); }

private:
  
  nsTArray<nsAutoPtr<Period> > mPeriods;

  
  nsTArray<nsString> mBaseUrls;
};

}
}

#endif 
