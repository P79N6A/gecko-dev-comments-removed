







#ifndef __SHARED_BREAKITERATOR_H__
#define __SHARED_BREAKITERATOR_H__

#include "unicode/utypes.h"
#include "sharedobject.h"

#if !UCONFIG_NO_BREAK_ITERATION

U_NAMESPACE_BEGIN

class BreakIterator;









class U_I18N_API SharedBreakIterator : public SharedObject {
public:
    SharedBreakIterator(BreakIterator *biToAdopt);
    virtual ~SharedBreakIterator();

    BreakIterator *get() const { return ptr; }
    BreakIterator *operator->() const { return ptr; }
    BreakIterator &operator*() const { return *ptr; }
private:
    BreakIterator *ptr;
    SharedBreakIterator(const SharedBreakIterator &);
    SharedBreakIterator &operator=(const SharedBreakIterator &);
};

U_NAMESPACE_END

#endif

#endif
