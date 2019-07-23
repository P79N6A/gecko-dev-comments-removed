




































#include "nsRangeCharsList.h"

nsRangeCharsList::nsRangeCharsList(PRUnichar aStart, PRUnichar aEnd)
{
   mStart = aStart;
   mEnd = aEnd;
}
nsRangeCharsList::~nsRangeCharsList()
{
}

PRUnichar nsRangeCharsList::Get( PRUint32 aIdx) 
{
   return mStart + aIdx;
}

    
PRUint32 nsRangeCharsList::Length() 
{
   return mEnd - mStart + 1;
}

