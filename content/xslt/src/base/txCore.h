





































#ifndef __txCore_h__
#define __txCore_h__

#include "nscore.h"
#include "nsDebug.h"
#include "prtypes.h"
#include "txDouble.h"

class nsAString;

class txObject
{
public:
    


    virtual ~txObject()
    {
    }
};




class txDouble
{
public:
    


    static const double NaN;
    static const double POSITIVE_INFINITY;
    static const double NEGATIVE_INFINITY;

    



    static PRBool isInfinite(double aDbl);

    


    static PRBool isNaN(double aDbl);

    


    static PRBool isNeg(double aDbl);

    



    static void toString(double aValue, nsAString& aDest);

    



    static double toDouble(const nsAString& aStr);
};


#define TxObject txObject
typedef txDouble Double;
typedef PRBool MBool;

#define MB_TRUE  PR_TRUE
#define MB_FALSE PR_FALSE


#endif
