








#ifndef UNIFILT_H
#define UNIFILT_H

#include "unicode/unifunct.h"
#include "unicode/unimatch.h"






U_NAMESPACE_BEGIN









#define U_ETHER ((UChar)0xFFFF)



























class U_COMMON_API UnicodeFilter : public UnicodeFunctor, public UnicodeMatcher {

public:
    



    virtual ~UnicodeFilter();

    






    virtual UBool contains(UChar32 c) const = 0;

    




    virtual UnicodeMatcher* toMatcher() const;

    



    virtual UMatchDegree matches(const Replaceable& text,
                                 int32_t& offset,
                                 int32_t limit,
                                 UBool incremental);

    



    virtual void setData(const TransliterationRuleData*);

    




    static UClassID U_EXPORT2 getStaticClassID();

protected:

    





};



U_NAMESPACE_END

#endif
