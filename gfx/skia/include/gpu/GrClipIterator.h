









#ifndef GrClipIterator_DEFINED
#define GrClipIterator_DEFINED

#include "GrRect.h"
#include "SkPath.h"
#include "SkRegion.h"




class GrClipIterator {
public:
    virtual ~GrClipIterator() {}

    


    virtual bool isDone() const = 0;

    


    virtual void rewind() = 0;

    


    virtual GrClipType getType() const = 0;

    



    virtual const SkPath* getPath() = 0;

    



    virtual GrPathFill getPathFill() const = 0;

    



    virtual void getRect(GrRect* rect) const = 0;

    



    virtual SkRegion::Op getOp() const = 0;

    


    virtual bool getDoAA() const = 0;

    



    virtual void next() = 0;
};




static inline void GrSafeRewind(GrClipIterator* iter) {
    if (iter) {
        iter->rewind();
    }
}

#endif

