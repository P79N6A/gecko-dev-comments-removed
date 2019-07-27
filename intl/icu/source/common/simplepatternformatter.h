







#ifndef __SIMPLEPATTERNFORMATTER_H__
#define __SIMPLEPATTERNFORMATTER_H__ 

#define EXPECTED_PLACEHOLDER_COUNT 3

#include "cmemory.h"
#include "unicode/utypes.h"
#include "unicode/unistr.h"

U_NAMESPACE_BEGIN

class SimplePatternFormatterPlaceholderValues;

struct PlaceholderInfo {
  int32_t id;
  int32_t offset;
};





















class U_COMMON_API SimplePatternFormatter : public UMemory {
public:
    


    SimplePatternFormatter();

    



    explicit SimplePatternFormatter(const UnicodeString& pattern);

    


    SimplePatternFormatter(const SimplePatternFormatter& other);

    


    SimplePatternFormatter &operator=(const SimplePatternFormatter& other);

    


    ~SimplePatternFormatter();

    






    UBool compile(const UnicodeString &pattern, UErrorCode &status);

    






    int32_t getPlaceholderCount() const {
        return placeholderCount;
    }

    


    const UnicodeString &getPatternWithNoPlaceholders() const {
        return noPlaceholders;
    }

    


    UnicodeString &format(
            const UnicodeString &args0,
            UnicodeString &appendTo,
            UErrorCode &status) const;
    
    


    UnicodeString &format(
            const UnicodeString &args0,
            const UnicodeString &args1,
            UnicodeString &appendTo,
            UErrorCode &status) const;
    
    


    UnicodeString &format(
            const UnicodeString &args0,
            const UnicodeString &args1,
            const UnicodeString &args2,
            UnicodeString &appendTo,
            UErrorCode &status) const;
    
    





















    UnicodeString &formatAndAppend(
            const UnicodeString * const *placeholderValues,
            int32_t placeholderValueCount,
            UnicodeString &appendTo,
            int32_t *offsetArray,
            int32_t offsetArrayLength,
            UErrorCode &status) const;

    























    UnicodeString &formatAndReplace(
            const UnicodeString * const *placeholderValues,
            int32_t placeholderValueCount,
            UnicodeString &result,
            int32_t *offsetArray,
            int32_t offsetArrayLength,
            UErrorCode &status) const;
private:
    UnicodeString noPlaceholders;
    MaybeStackArray<PlaceholderInfo, 3> placeholders;
    int32_t placeholderSize;
    int32_t placeholderCount;
    UBool firstPlaceholderReused;

    
    
    UnicodeString &formatAndAppend(
            const SimplePatternFormatterPlaceholderValues &placeholderValues,
            UnicodeString &appendTo,
            int32_t *offsetArray,
            int32_t offsetArrayLength) const;

    
    
    
    
    int32_t getUniquePlaceholderAtStart() const;
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    int32_t ensureCapacity(int32_t desiredCapacity, int32_t allocationSize=0);

    
    
    UBool addPlaceholder(int32_t id, int32_t offset);
};

U_NAMESPACE_END

#endif
