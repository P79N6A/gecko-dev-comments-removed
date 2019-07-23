





































 
#ifndef NSEPSOBJECTPS_H
#define NSEPSOBJECTPS_H

#include <stdio.h>

#include "nscore.h"
#include "prtypes.h"
#include "nsString.h"

class nsEPSObjectPS {
    public:
        


        nsEPSObjectPS(FILE *aFile);

        




        nsresult GetStatus() { return mStatus; };

        



        PRFloat64 GetBoundingBoxLLX() { return mBBllx; };
        PRFloat64 GetBoundingBoxLLY() { return mBBlly; };
        PRFloat64 GetBoundingBoxURX() { return mBBurx; };
        PRFloat64 GetBoundingBoxURY() { return mBBury; };

        




        nsresult WriteTo(FILE *aDest);

    private:
        nsresult        mStatus;
        FILE *          mEPSF;
        PRFloat64       mBBllx,
                        mBBlly,
                        mBBurx,
                        mBBury;

        void            Parse();
        PRBool          EPSFFgets(nsACString& aBuffer);
};

#endif 

