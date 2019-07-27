






#ifndef SkRecordDraw_DEFINED
#define SkRecordDraw_DEFINED

#include "SkRecord.h"
#include "SkCanvas.h"
#include "SkDrawPictureCallback.h"


void SkRecordDraw(const SkRecord&, SkCanvas*, SkDrawPictureCallback* = NULL);

namespace SkRecords {


class Draw : SkNoncopyable {
public:
    explicit Draw(SkCanvas* canvas)
        : fInitialCTM(canvas->getTotalMatrix()), fCanvas(canvas), fIndex(0) {}

    unsigned index() const { return fIndex; }
    void next() { ++fIndex; }

    template <typename T> void operator()(const T& r) {
        if (!this->skip(r)) {
            this->draw(r);
        }
    }

private:
    
    template <typename T> void draw(const T&);

    
    

    
    template <typename T> bool skip(const T&) { return false; }

    
    bool skip(const PairedPushCull&);
    bool skip(const BoundedDrawPosTextH&);

    const SkMatrix fInitialCTM;
    SkCanvas* fCanvas;
    unsigned fIndex;
};

}  

#endif
