








#ifndef SkRegionPriv_DEFINED
#define SkRegionPriv_DEFINED

#include "SkRegion.h"
#include "SkThread.h"

#define assert_sentinel(value, isSentinel) \
    SkASSERT(((value) == SkRegion::kRunTypeSentinel) == isSentinel)



#ifdef SK_DEBUG



static int compute_intervalcount(const SkRegion::RunType runs[]) {
    const SkRegion::RunType* curr = runs;
    while (*curr < SkRegion::kRunTypeSentinel) {
        SkASSERT(curr[0] < curr[1]);
        SkASSERT(curr[1] < SkRegion::kRunTypeSentinel);
        curr += 2;
    }
    return SkToInt((curr - runs) >> 1);
}
#endif

struct SkRegion::RunHead {
private:

public:
    int32_t fRefCnt;
    int32_t fRunCount;

    





    int getYSpanCount() const {
        return fYSpanCount;
    }

    




    int getIntervalCount() const {
        return fIntervalCount;
    }

    static RunHead* Alloc(int count) {
        
        

        SkASSERT(count >= SkRegion::kRectRegionRuns);

        RunHead* head = (RunHead*)sk_malloc_throw(sizeof(RunHead) + count * sizeof(RunType));
        head->fRefCnt = 1;
        head->fRunCount = count;
        
        head->fYSpanCount = 0;
        head->fIntervalCount = 0;
        return head;
    }

    static RunHead* Alloc(int count, int yspancount, int intervalCount) {
        SkASSERT(yspancount > 0);
        SkASSERT(intervalCount > 1);

        RunHead* head = Alloc(count);
        head->fYSpanCount = yspancount;
        head->fIntervalCount = intervalCount;
        return head;
    }

    SkRegion::RunType* writable_runs() {
        SkASSERT(fRefCnt == 1);
        return (SkRegion::RunType*)(this + 1);
    }

    const SkRegion::RunType* readonly_runs() const {
        return (const SkRegion::RunType*)(this + 1);
    }

    RunHead* ensureWritable() {
        RunHead* writable = this;
        if (fRefCnt > 1) {
            
            
            
            writable = Alloc(fRunCount, fYSpanCount, fIntervalCount);
            memcpy(writable->writable_runs(), this->readonly_runs(),
                   fRunCount * sizeof(RunType));

            
            
            
            if (sk_atomic_dec(&fRefCnt) == 1) {
                sk_free(this);
            }
        }
        return writable;
    }

    



    static SkRegion::RunType* SkipEntireScanline(const SkRegion::RunType runs[]) {
        
        SkASSERT(runs[0] < SkRegion::kRunTypeSentinel);

        const int intervals = runs[1];
        SkASSERT(runs[2 + intervals * 2] == SkRegion::kRunTypeSentinel);
#ifdef SK_DEBUG
        {
            int n = compute_intervalcount(&runs[2]);
            SkASSERT(n == intervals);
        }
#endif

        
        runs += 1 + 1 + intervals * 2 + 1;
        return const_cast<SkRegion::RunType*>(runs);
    }


    






    SkRegion::RunType* findScanline(int y) const {
        const RunType* runs = this->readonly_runs();

        
        SkASSERT(y >= runs[0]);

        runs += 1;  
        for (;;) {
            int bottom = runs[0];
            
            
            SkASSERT(bottom < SkRegion::kRunTypeSentinel);
            if (y < bottom) {
                break;
            }
            runs = SkipEntireScanline(runs);
        }
        return const_cast<SkRegion::RunType*>(runs);
    }

    
    void computeRunBounds(SkIRect* bounds) {
        RunType* runs = this->writable_runs();
        bounds->fTop = *runs++;

        int bot;
        int ySpanCount = 0;
        int intervalCount = 0;
        int left = SK_MaxS32;
        int rite = SK_MinS32;

        do {
            bot = *runs++;
            SkASSERT(bot < SkRegion::kRunTypeSentinel);
            ySpanCount += 1;

            const int intervals = *runs++;
            SkASSERT(intervals >= 0);
            SkASSERT(intervals < SkRegion::kRunTypeSentinel);

            if (intervals > 0) {
#ifdef SK_DEBUG
                {
                    int n = compute_intervalcount(runs);
                    SkASSERT(n == intervals);
                }
#endif
                RunType L = runs[0];
                SkASSERT(L < SkRegion::kRunTypeSentinel);
                if (left > L) {
                    left = L;
                }

                runs += intervals * 2;
                RunType R = runs[-1];
                SkASSERT(R < SkRegion::kRunTypeSentinel);
                if (rite < R) {
                    rite = R;
                }

                intervalCount += intervals;
            }
            SkASSERT(SkRegion::kRunTypeSentinel == *runs);
            runs += 1;  

            
        } while (SkRegion::kRunTypeSentinel > *runs);

#ifdef SK_DEBUG
        
        int runCount = SkToInt(runs - this->writable_runs() + 1);
        SkASSERT(runCount == fRunCount);
#endif

        fYSpanCount = ySpanCount;
        fIntervalCount = intervalCount;

        bounds->fLeft = left;
        bounds->fRight = rite;
        bounds->fBottom = bot;
    }

private:
    int32_t fYSpanCount;
    int32_t fIntervalCount;
};

#endif
