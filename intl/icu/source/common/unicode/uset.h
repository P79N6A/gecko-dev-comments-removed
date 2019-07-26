

























#ifndef __USET_H__
#define __USET_H__

#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/localpointer.h"

#ifndef UCNV_H
struct USet;





typedef struct USet USet;
#endif






enum {
    



    USET_IGNORE_SPACE = 1,  

    

























    USET_CASE_INSENSITIVE = 2,  

    







    USET_ADD_CASE_MAPPINGS = 4
};
























































typedef enum USetSpanCondition {
    










    USET_SPAN_NOT_CONTAINED = 0,
    













    USET_SPAN_CONTAINED = 1,
    

















    USET_SPAN_SIMPLE = 2,
    



    USET_SPAN_CONDITION_COUNT
} USetSpanCondition;

enum {
    





    USET_SERIALIZED_STATIC_ARRAY_CAPACITY=8
};






typedef struct USerializedSet {
    



    const uint16_t *array;
    



    int32_t bmpLength;
    



    int32_t length;
    



    uint16_t staticArray[USET_SERIALIZED_STATIC_ARRAY_CAPACITY];
} USerializedSet;












U_STABLE USet* U_EXPORT2
uset_openEmpty(void);











U_STABLE USet* U_EXPORT2
uset_open(UChar32 start, UChar32 end);










U_STABLE USet* U_EXPORT2
uset_openPattern(const UChar* pattern, int32_t patternLength,
                 UErrorCode* ec);












U_STABLE USet* U_EXPORT2
uset_openPatternOptions(const UChar* pattern, int32_t patternLength,
                 uint32_t options,
                 UErrorCode* ec);







U_STABLE void U_EXPORT2
uset_close(USet* set);

#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN










U_DEFINE_LOCAL_OPEN_POINTER(LocalUSetPointer, USet, uset_close);

U_NAMESPACE_END

#endif










U_STABLE USet * U_EXPORT2
uset_clone(const USet *set);










U_STABLE UBool U_EXPORT2
uset_isFrozen(const USet *set);















U_STABLE void U_EXPORT2
uset_freeze(USet *set);











U_STABLE USet * U_EXPORT2
uset_cloneAsThawed(const USet *set);










U_STABLE void U_EXPORT2
uset_set(USet* set,
         UChar32 start, UChar32 end);






















U_STABLE int32_t U_EXPORT2 
uset_applyPattern(USet *set,
                  const UChar *pattern, int32_t patternLength,
                  uint32_t options,
                  UErrorCode *status);























U_STABLE void U_EXPORT2
uset_applyIntPropertyValue(USet* set,
                           UProperty prop, int32_t value, UErrorCode* ec);




































U_STABLE void U_EXPORT2
uset_applyPropertyAlias(USet* set,
                        const UChar *prop, int32_t propLength,
                        const UChar *value, int32_t valueLength,
                        UErrorCode* ec);










U_STABLE UBool U_EXPORT2
uset_resemblesPattern(const UChar *pattern, int32_t patternLength,
                      int32_t pos);
















U_STABLE int32_t U_EXPORT2
uset_toPattern(const USet* set,
               UChar* result, int32_t resultCapacity,
               UBool escapeUnprintable,
               UErrorCode* ec);









U_STABLE void U_EXPORT2
uset_add(USet* set, UChar32 c);













U_STABLE void U_EXPORT2
uset_addAll(USet* set, const USet *additionalSet);










U_STABLE void U_EXPORT2
uset_addRange(USet* set, UChar32 start, UChar32 end);










U_STABLE void U_EXPORT2
uset_addString(USet* set, const UChar* str, int32_t strLen);










U_STABLE void U_EXPORT2
uset_addAllCodePoints(USet* set, const UChar *str, int32_t strLen);









U_STABLE void U_EXPORT2
uset_remove(USet* set, UChar32 c);










U_STABLE void U_EXPORT2
uset_removeRange(USet* set, UChar32 start, UChar32 end);










U_STABLE void U_EXPORT2
uset_removeString(USet* set, const UChar* str, int32_t strLen);












U_STABLE void U_EXPORT2
uset_removeAll(USet* set, const USet* removeSet);















U_STABLE void U_EXPORT2
uset_retain(USet* set, UChar32 start, UChar32 end);













U_STABLE void U_EXPORT2
uset_retainAll(USet* set, const USet* retain);









U_STABLE void U_EXPORT2
uset_compact(USet* set);









U_STABLE void U_EXPORT2
uset_complement(USet* set);












U_STABLE void U_EXPORT2
uset_complementAll(USet* set, const USet* complement);








U_STABLE void U_EXPORT2
uset_clear(USet* set);



























U_STABLE void U_EXPORT2
uset_closeOver(USet* set, int32_t attributes);







U_STABLE void U_EXPORT2
uset_removeAllStrings(USet* set);








U_STABLE UBool U_EXPORT2
uset_isEmpty(const USet* set);









U_STABLE UBool U_EXPORT2
uset_contains(const USet* set, UChar32 c);










U_STABLE UBool U_EXPORT2
uset_containsRange(const USet* set, UChar32 start, UChar32 end);









U_STABLE UBool U_EXPORT2
uset_containsString(const USet* set, const UChar* str, int32_t strLen);











U_STABLE int32_t U_EXPORT2
uset_indexOf(const USet* set, UChar32 c);











U_STABLE UChar32 U_EXPORT2
uset_charAt(const USet* set, int32_t charIndex);









U_STABLE int32_t U_EXPORT2
uset_size(const USet* set);









U_STABLE int32_t U_EXPORT2
uset_getItemCount(const USet* set);



















U_STABLE int32_t U_EXPORT2
uset_getItem(const USet* set, int32_t itemIndex,
             UChar32* start, UChar32* end,
             UChar* str, int32_t strCapacity,
             UErrorCode* ec);









U_STABLE UBool U_EXPORT2
uset_containsAll(const USet* set1, const USet* set2);











U_STABLE UBool U_EXPORT2
uset_containsAllCodePoints(const USet* set, const UChar *str, int32_t strLen);









U_STABLE UBool U_EXPORT2
uset_containsNone(const USet* set1, const USet* set2);









U_STABLE UBool U_EXPORT2
uset_containsSome(const USet* set1, const USet* set2);




















U_STABLE int32_t U_EXPORT2
uset_span(const USet *set, const UChar *s, int32_t length, USetSpanCondition spanCondition);



















U_STABLE int32_t U_EXPORT2
uset_spanBack(const USet *set, const UChar *s, int32_t length, USetSpanCondition spanCondition);




















U_STABLE int32_t U_EXPORT2
uset_spanUTF8(const USet *set, const char *s, int32_t length, USetSpanCondition spanCondition);



















U_STABLE int32_t U_EXPORT2
uset_spanBackUTF8(const USet *set, const char *s, int32_t length, USetSpanCondition spanCondition);









U_STABLE UBool U_EXPORT2
uset_equals(const USet* set1, const USet* set2);






















































U_STABLE int32_t U_EXPORT2
uset_serialize(const USet* set, uint16_t* dest, int32_t destCapacity, UErrorCode* pErrorCode);









U_STABLE UBool U_EXPORT2
uset_getSerializedSet(USerializedSet* fillSet, const uint16_t* src, int32_t srcLength);








U_STABLE void U_EXPORT2
uset_setSerializedToOne(USerializedSet* fillSet, UChar32 c);









U_STABLE UBool U_EXPORT2
uset_serializedContains(const USerializedSet* set, UChar32 c);










U_STABLE int32_t U_EXPORT2
uset_getSerializedRangeCount(const USerializedSet* set);














U_STABLE UBool U_EXPORT2
uset_getSerializedRange(const USerializedSet* set, int32_t rangeIndex,
                        UChar32* pStart, UChar32* pEnd);

#endif
