




































#ifndef nsTime_h__
#define nsTime_h__

#include "prtime.h"
#include "nscore.h"










class nsTime
{
public:
    


    nsTime(void) : mValue(PR_Now()) {
    }

    


    nsTime(const char* dateStr, PRBool defaultToGMT) {
        PRInt64 theTime;
        PRStatus status = PR_ParseTimeString(dateStr, defaultToGMT, &theTime);
        if (status == PR_SUCCESS)
            mValue = theTime;
        else
            mValue = LL_ZERO;
    }

    


    nsTime(const PRTime aTime) : mValue(aTime) {
    }

    

    


    operator PRTime(void) const {
        return mValue;
    }

    PRInt64 mValue;
};




inline const PRBool
operator <(const nsTime& aTime1, const nsTime& aTime2) {
    return aTime1.mValue < aTime2.mValue;
}




inline const PRBool
operator <=(const nsTime& aTime1, const nsTime& aTime2) {
    return aTime1.mValue <= aTime2.mValue;
}




inline const PRBool
operator >(const nsTime& aTime1, const nsTime& aTime2) {
    return aTime1.mValue > aTime2.mValue;
}




inline const PRBool
operator >=(const nsTime& aTime1, const nsTime& aTime2) {
    return aTime1.mValue >= aTime2.mValue;
}

#endif 
