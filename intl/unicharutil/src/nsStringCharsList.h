



































#ifndef nsStringCharsList_h__
#define nsStringCharsList_h__


#include "nscore.h"
#include "nsString.h"
#include "nsCharsList.h"

class nsStringCharsList : public nsCharsList {
   
public: 
   nsStringCharsList(nsString&  aList);
   virtual ~nsStringCharsList();

   virtual PRUnichar Get( PRUint32 aIdx) ;
    
   virtual PRUint32 Length() ;

private:
   nsString mList;
};

#endif  
