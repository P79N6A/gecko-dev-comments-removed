










#ifndef __DTINTRV_H__
#define __DTINTRV_H__

#include "unicode/utypes.h"
#include "unicode/uobject.h"







U_NAMESPACE_BEGIN







class U_COMMON_API DateInterval : public UObject {
public:

    





    DateInterval(UDate fromDate, UDate toDate);

    



    virtual ~DateInterval();
 
    




    UDate getFromDate() const;

    




    UDate getToDate() const;


    










    static UClassID U_EXPORT2 getStaticClassID(void);

    










    virtual UClassID getDynamicClassID(void) const;

    
    



    DateInterval(const DateInterval& other);

    



    DateInterval& operator=(const DateInterval&);

    




    virtual UBool operator==(const DateInterval& other) const;

    




    UBool operator!=(const DateInterval& other) const;


    





     virtual DateInterval* clone() const;

private:
    


    DateInterval();

    UDate fromDate;
    UDate toDate;

} ;


inline UDate 
DateInterval::getFromDate() const { 
    return fromDate; 
}


inline UDate 
DateInterval::getToDate() const { 
    return toDate; 
}


inline UBool 
DateInterval::operator!=(const DateInterval& other) const { 
    return ( !operator==(other) );
}


U_NAMESPACE_END

#endif
