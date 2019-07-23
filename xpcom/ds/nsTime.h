




































#ifndef nsTime_h__
#define nsTime_h__

#include "prtime.h"
#include "nsInt64.h"
#include "nscore.h"










class nsTime : public nsInt64
{
public:
    


    nsTime(void) : nsInt64(PR_Now()) {
    }

    


    nsTime(const char* dateStr, PRBool defaultToGMT) {
        PRInt64 theTime;
        PRStatus status = PR_ParseTimeString(dateStr, defaultToGMT, &theTime);
        if (status == PR_SUCCESS)
            mValue = theTime;
        else
            mValue = LL_ZERO;
    }

    


    nsTime(const PRTime aTime) : nsInt64(aTime) {
    }

    


    nsTime(const nsInt64& aTime) : nsInt64(aTime) {
    }

    


    nsTime(const nsTime& aTime) : nsInt64(aTime.mValue) {
    }

    

    


    const nsTime& operator =(const nsTime& aTime) {
        mValue = aTime.mValue;
        return *this;
    }

    


    operator PRTime(void) const {
        return mValue;
    }
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
