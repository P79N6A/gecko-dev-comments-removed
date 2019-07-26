








#ifndef UNIFUNCT_H
#define UNIFUNCT_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"





 
U_NAMESPACE_BEGIN

class UnicodeMatcher;
class UnicodeReplacer;
class TransliterationRuleData;







class U_COMMON_API UnicodeFunctor : public UObject {

public:

    



    virtual ~UnicodeFunctor();

    





    virtual UnicodeFunctor* clone() const = 0;

    









    virtual UnicodeMatcher* toMatcher() const;

    









    virtual UnicodeReplacer* toReplacer() const;

    





    static UClassID U_EXPORT2 getStaticClassID(void);

    














    virtual UClassID getDynamicClassID(void) const = 0;

    







    virtual void setData(const TransliterationRuleData*) = 0;

protected:

    




    

};



U_NAMESPACE_END

#endif
