




#ifndef __txCore_h__
#define __txCore_h__

#include "nscore.h"
#include "nsDebug.h"
#include "nsISupportsImpl.h"

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
    



    static void toString(double aValue, nsAString& aDest);

    



    static double toDouble(const nsAString& aStr);
};

#endif
