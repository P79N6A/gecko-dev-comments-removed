
















#ifndef PERIOD_H_
#define PERIOD_H_

#include "nsTArray.h"
#include "AdaptationSet.h"
#include "Representation.h"

























#include "nsAutoPtr.h"

namespace mozilla {
namespace net {

class Period
{
public:
  Period()
  {
    MOZ_COUNT_CTOR(Period);
  }
  virtual ~Period() {
    MOZ_COUNT_DTOR(Period);
  }

  
  AdaptationSet const * GetAdaptationSet(uint32_t aIndex) const;
  
  void                  AddAdaptationSet(AdaptationSet* aAdaptationSet);

  
  uint16_t const        GetNumAdaptationSets() const;

  
  double const GetStart() const;
  void SetStart(double const aStart);

  
  double const GetDuration() const;
  void SetDuration(double const aDuration);

private:
  
  nsTArray<nsAutoPtr<AdaptationSet> > mAdaptationSets;

  
  double mStart;

  
  double mDuration;
};

}
}


#endif 
