



































#ifndef nsCharsList_h__
#define nsCharsList_h__


#include "nscore.h"

class nsCharsList {
   
public: 

   virtual PRUnichar Get( PRUint32 aIdx) = 0;
    
   virtual PRUint32 Length() = 0;
};

#endif  
