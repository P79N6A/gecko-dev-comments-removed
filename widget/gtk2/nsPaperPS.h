





































 
#ifndef _PAPERPS_H_
#define _PAPERPS_H_

#include "prtypes.h"
#include "nsDebug.h"

struct nsPaperSizePS_ {
    const char *name;
    float width_mm;
    float height_mm;
    bool isMetric;        
};

class nsPaperSizePS {
    public:
        


        nsPaperSizePS() { mCurrent = 0; }

        


        bool AtEnd() { return mCurrent >= mCount; }

        



        void First() { mCurrent = 0; }

        



        void Next() {
            NS_ASSERTION(!AtEnd(), "Invalid current item");
            mCurrent++;
        }

        



        bool Find(const char *aName);

        


        const char *Name() {
            NS_PRECONDITION(!AtEnd(), "Invalid current item");
            return mList[mCurrent].name;
        }

        


        float Width_mm() {
            NS_PRECONDITION(!AtEnd(), "Invalid current item");
            return mList[mCurrent].width_mm;
        }

        


        float Height_mm() {
            NS_PRECONDITION(!AtEnd(), "Invalid current item");
            return mList[mCurrent].height_mm;
        }

        



        bool IsMetric() {
            NS_PRECONDITION(!AtEnd(), "Invalid current item");
            return mList[mCurrent].isMetric;
        }

    private:
        unsigned int mCurrent;
        static const nsPaperSizePS_ mList[];
        static const unsigned int mCount;
};

#endif

