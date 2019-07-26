








#ifndef SkAntiRun_DEFINED
#define SkAntiRun_DEFINED

#include "SkBlitter.h"






class SkAlphaRuns {
public:
    int16_t*    fRuns;
    uint8_t*     fAlpha;

    
    
    bool empty() const {
        SkASSERT(fRuns[0] > 0);
        return fAlpha[0] == 0 && fRuns[fRuns[0]] == 0;
    }

    
    void    reset(int width);

    












    SK_ALWAYS_INLINE int add(int x, U8CPU startAlpha, int middleCount, U8CPU stopAlpha,
                             U8CPU maxValue, int offsetX) {
        SkASSERT(middleCount >= 0);
        SkASSERT(x >= 0 && x + (startAlpha != 0) + middleCount + (stopAlpha != 0) <= fWidth);

        SkASSERT(fRuns[offsetX] >= 0);

        int16_t*    runs = fRuns + offsetX;
        uint8_t*    alpha = fAlpha + offsetX;
        uint8_t*    lastAlpha = alpha;
        x -= offsetX;

        if (startAlpha) {
            SkAlphaRuns::Break(runs, alpha, x, 1);
            




            unsigned tmp = alpha[x] + startAlpha;
            SkASSERT(tmp <= 256);
            alpha[x] = SkToU8(tmp - (tmp >> 8));    

            runs += x + 1;
            alpha += x + 1;
            x = 0;
            lastAlpha += x; 
            SkDEBUGCODE(this->validate();)
        }

        if (middleCount) {
            SkAlphaRuns::Break(runs, alpha, x, middleCount);
            alpha += x;
            runs += x;
            x = 0;
            do {
                alpha[0] = SkToU8(alpha[0] + maxValue);
                int n = runs[0];
                SkASSERT(n <= middleCount);
                alpha += n;
                runs += n;
                middleCount -= n;
            } while (middleCount > 0);
            SkDEBUGCODE(this->validate();)
            lastAlpha = alpha;
        }

        if (stopAlpha) {
            SkAlphaRuns::Break(runs, alpha, x, 1);
            alpha += x;
            alpha[0] = SkToU8(alpha[0] + stopAlpha);
            SkDEBUGCODE(this->validate();)
            lastAlpha = alpha;
        }

        return SkToS32(lastAlpha - fAlpha);  
    }

    SkDEBUGCODE(void assertValid(int y, int maxStep) const;)
    SkDEBUGCODE(void dump() const;)

    







    static void Break(int16_t runs[], uint8_t alpha[], int x, int count) {
        SkASSERT(count > 0 && x >= 0);

        
        

        int16_t* next_runs = runs + x;
        uint8_t*  next_alpha = alpha + x;

        while (x > 0) {
            int n = runs[0];
            SkASSERT(n > 0);

            if (x < n) {
                alpha[x] = alpha[0];
                runs[0] = SkToS16(x);
                runs[x] = SkToS16(n - x);
                break;
            }
            runs += n;
            alpha += n;
            x -= n;
        }

        runs = next_runs;
        alpha = next_alpha;
        x = count;

        for (;;) {
            int n = runs[0];
            SkASSERT(n > 0);

            if (x < n) {
                alpha[x] = alpha[0];
                runs[0] = SkToS16(x);
                runs[x] = SkToS16(n - x);
                break;
            }
            x -= n;
            if (x <= 0) {
                break;
            }
            runs += n;
            alpha += n;
        }
    }

    





    static void BreakAt(int16_t runs[], uint8_t alpha[], int x) {
        while (x > 0) {
            int n = runs[0];
            SkASSERT(n > 0);

            if (x < n) {
                alpha[x] = alpha[0];
                runs[0] = SkToS16(x);
                runs[x] = SkToS16(n - x);
                break;
            }
            runs += n;
            alpha += n;
            x -= n;
        }
    }

private:
    SkDEBUGCODE(int fWidth;)
    SkDEBUGCODE(void validate() const;)
};

#endif
