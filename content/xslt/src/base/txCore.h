





































#ifndef __txCore_h__
#define __txCore_h__

#include "nsContentUtils.h"
#include "nscore.h"
#include "nsDebug.h"
#include "nsTraceRefcnt.h"
#include "prtypes.h"

class nsAString;

class txObject
{
public:
    txObject()
    {
        MOZ_COUNT_CTOR(txObject);
    }

    


    virtual ~txObject()
    {
        MOZ_COUNT_DTOR(txObject);
    }
};




class txDouble
{
public:
    


    static const dpun NaN;
    static const dpun POSITIVE_INFINITY;
    static const dpun NEGATIVE_INFINITY;

    



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
