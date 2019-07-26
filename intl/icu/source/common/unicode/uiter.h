















#ifndef __UITER_H__
#define __UITER_H__








#include "unicode/utypes.h"

#if U_SHOW_CPLUSPLUS_API
    U_NAMESPACE_BEGIN

    class CharacterIterator;
    class Replaceable;

    U_NAMESPACE_END
#endif

U_CDECL_BEGIN

struct UCharIterator;
typedef struct UCharIterator UCharIterator; 







typedef enum UCharIteratorOrigin {
    UITER_START, UITER_CURRENT, UITER_LIMIT, UITER_ZERO, UITER_LENGTH
} UCharIteratorOrigin;


enum {
    













    UITER_UNKNOWN_INDEX=-2
};













#define UITER_NO_STATE ((uint32_t)0xffffffff)



















typedef int32_t U_CALLCONV
UCharIteratorGetIndex(UCharIterator *iter, UCharIteratorOrigin origin);



































typedef int32_t U_CALLCONV
UCharIteratorMove(UCharIterator *iter, int32_t delta, UCharIteratorOrigin origin);













typedef UBool U_CALLCONV
UCharIteratorHasNext(UCharIterator *iter);












typedef UBool U_CALLCONV
UCharIteratorHasPrevious(UCharIterator *iter);
 












typedef UChar32 U_CALLCONV
UCharIteratorCurrent(UCharIterator *iter);














typedef UChar32 U_CALLCONV
UCharIteratorNext(UCharIterator *iter);














typedef UChar32 U_CALLCONV
UCharIteratorPrevious(UCharIterator *iter);












typedef int32_t U_CALLCONV
UCharIteratorReserved(UCharIterator *iter, int32_t something);













































typedef uint32_t U_CALLCONV
UCharIteratorGetState(const UCharIterator *iter);


























typedef void U_CALLCONV
UCharIteratorSetState(UCharIterator *iter, uint32_t state, UErrorCode *pErrorCode);































struct UCharIterator {
    




    const void *context;

    




    int32_t length;

    




    int32_t start;

    




    int32_t index;

    




    int32_t limit;

    



    int32_t reservedField;

    






    UCharIteratorGetIndex *getIndex;

    








    UCharIteratorMove *move;

    






    UCharIteratorHasNext *hasNext;

    





    UCharIteratorHasPrevious *hasPrevious;

    






    UCharIteratorCurrent *current;

    







    UCharIteratorNext *next;

    







    UCharIteratorPrevious *previous;

    





    UCharIteratorReserved *reservedFn;

    






    UCharIteratorGetState *getState;

    







    UCharIteratorSetState *setState;
};



















U_STABLE UChar32 U_EXPORT2
uiter_current32(UCharIterator *iter);















U_STABLE UChar32 U_EXPORT2
uiter_next32(UCharIterator *iter);















U_STABLE UChar32 U_EXPORT2
uiter_previous32(UCharIterator *iter);



















U_STABLE uint32_t U_EXPORT2
uiter_getState(const UCharIterator *iter);
















U_STABLE void U_EXPORT2
uiter_setState(UCharIterator *iter, uint32_t state, UErrorCode *pErrorCode);























U_STABLE void U_EXPORT2
uiter_setString(UCharIterator *iter, const UChar *s, int32_t length);





















U_STABLE void U_EXPORT2
uiter_setUTF16BE(UCharIterator *iter, const char *s, int32_t length);


































U_STABLE void U_EXPORT2
uiter_setUTF8(UCharIterator *iter, const char *s, int32_t length);

#if U_SHOW_CPLUSPLUS_API





















U_STABLE void U_EXPORT2
uiter_setCharacterIterator(UCharIterator *iter, icu::CharacterIterator *charIter);























U_STABLE void U_EXPORT2
uiter_setReplaceable(UCharIterator *iter, const icu::Replaceable *rep);

#endif

U_CDECL_END

#endif
