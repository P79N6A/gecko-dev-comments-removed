



































#ifndef nsRangeCharsList_h__
#define nsRangeCharsList_h__


#include "nsCharsList.h"

class nsRangeCharsList : public nsCharsList {
   
public: 
   nsRangeCharsList(PRUnichar aStart, PRUnichar aEnd);
   virtual ~nsRangeCharsList();

   virtual PRUnichar Get( PRUint32 aIdx) ;
    
   virtual PRUint32 Length() ;

private:
   PRUnichar mStart;
   PRUnichar mEnd;
};

#endif  
