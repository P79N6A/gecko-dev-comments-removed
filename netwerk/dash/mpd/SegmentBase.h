







































#ifndef SEGMENTBASE_H_
#define SEGMENTBASE_H_

#include "nsString.h"

namespace mozilla {
namespace net {

class SegmentBase
{
public:
  SegmentBase() :
    mInitRangeStart(0),
    mInitRangeEnd(0),
    mIndexRangeStart(0),
    mIndexRangeEnd(0)
  {
    MOZ_COUNT_CTOR(SegmentBase);
  }
  virtual ~SegmentBase()
  {
    MOZ_COUNT_DTOR(SegmentBase);
  }

  bool operator==(SegmentBase const & other) const {
    return (mInitRangeStart == other.mInitRangeStart
            && mInitRangeEnd == other.mInitRangeEnd
            && mIndexRangeStart == other.mIndexRangeStart
            && mIndexRangeEnd == other.mIndexRangeEnd);
  }
  bool operator!=(SegmentBase const & other) const {
    return !(*this == other);
  }

  
  void GetInitRange(int64_t* aStartBytes, int64_t* aEndBytes) const;
  void SetInitRange(nsAString const &aRangeStr);

  
  void GetIndexRange(int64_t* aStartBytes, int64_t* aEndBytes) const;
  void SetIndexRange(nsAString const &aRangeStr);

private:
  
  void SetRange(nsAString const &aRangeStr, int64_t &aStart, int64_t &aEnd);

  
  int64_t mInitRangeStart;
  int64_t mInitRangeEnd;

  
  int64_t mIndexRangeStart;
  int64_t mIndexRangeEnd;
};


}
}

#endif 
