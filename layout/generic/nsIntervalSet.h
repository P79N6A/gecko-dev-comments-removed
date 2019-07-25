







































#ifndef nsIntervalSet_h___
#define nsIntervalSet_h___

#include "prtypes.h"
#include "nsCoord.h"
#include "nsDebug.h"

typedef void *
(* IntervalSetAlloc)(size_t aBytes, void *aClosure);

typedef void
(* IntervalSetFree) (size_t aBytes, void *aPtr, void *aClosure);





class nsIntervalSet {

public:

    typedef nscoord coord_type;

    nsIntervalSet(IntervalSetAlloc aAlloc, IntervalSetFree aFree,
                  void* aAllocatorClosure);
    ~nsIntervalSet();

    







    void IncludeInterval(coord_type aBegin, coord_type aEnd);

    



    bool Intersects(coord_type aBegin, coord_type aEnd) const;

    



    bool Contains(coord_type aBegin, coord_type aEnd) const;

    bool IsEmpty() const
    {
        return !mList;
    }

private:

    class Interval {

    public:
        Interval(coord_type aBegin, coord_type aEnd)
            : mBegin(aBegin),
              mEnd(aEnd),
              mPrev(nsnull),
              mNext(nsnull)
        {
        }

        coord_type mBegin;
        coord_type mEnd;
        Interval *mPrev;
        Interval *mNext;
    };

    void FreeInterval(Interval *aInterval);

    Interval           *mList;
    IntervalSetAlloc    mAlloc;
    IntervalSetFree     mFree;
    void               *mAllocatorClosure;
        
};

#endif 
