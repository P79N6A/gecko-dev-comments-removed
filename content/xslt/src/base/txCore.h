





































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

    



    static bool isInfinite(double aDbl);

    


    static bool isNaN(double aDbl);

    


    static bool isNeg(double aDbl);

    



    static void toString(double aValue, nsAString& aDest);

    



    static double toDouble(const nsAString& aStr);
};


#define TxObject txObject
typedef txDouble Double;
typedef bool MBool;

#define MB_TRUE  true
#define MB_FALSE false


#endif
