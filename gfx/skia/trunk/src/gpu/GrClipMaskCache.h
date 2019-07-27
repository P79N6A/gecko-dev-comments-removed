






#ifndef GrClipMaskCache_DEFINED
#define GrClipMaskCache_DEFINED

#include "GrContext.h"
#include "SkClipStack.h"
#include "SkTypes.h"

class GrTexture;





class GrClipMaskCache : SkNoncopyable {
public:
    GrClipMaskCache();

    ~GrClipMaskCache() {

        while (!fStack.empty()) {
            GrClipStackFrame* temp = (GrClipStackFrame*) fStack.back();
            temp->~GrClipStackFrame();
            fStack.pop_back();
        }
    }

    bool canReuse(int32_t clipGenID, const SkIRect& bounds) {

        SkASSERT(clipGenID != SkClipStack::kWideOpenGenID);
        SkASSERT(clipGenID != SkClipStack::kEmptyGenID);

        GrClipStackFrame* back = (GrClipStackFrame*) fStack.back();

        
        
        if (back->fLastMask.texture() &&
            back->fLastBound == bounds &&
            back->fLastClipGenID == clipGenID) {
            return true;
        }

        return false;
    }

    void reset() {
        if (fStack.empty()) {

            return;
        }

        GrClipStackFrame* back = (GrClipStackFrame*) fStack.back();

        back->reset();
    }

    





    void push();

    void pop() {
        

        if (!fStack.empty()) {
            GrClipStackFrame* back = (GrClipStackFrame*) fStack.back();

            back->~GrClipStackFrame();
            fStack.pop_back();
        }
    }

    int32_t getLastClipGenID() const {

        if (fStack.empty()) {
            return SkClipStack::kInvalidGenID;
        }

        return ((GrClipStackFrame*) fStack.back())->fLastClipGenID;
    }

    GrTexture* getLastMask() {

        if (fStack.empty()) {
            SkASSERT(false);
            return NULL;
        }

        GrClipStackFrame* back = (GrClipStackFrame*) fStack.back();

        return back->fLastMask.texture();
    }

    const GrTexture* getLastMask() const {

        if (fStack.empty()) {
            SkASSERT(false);
            return NULL;
        }

        GrClipStackFrame* back = (GrClipStackFrame*) fStack.back();

        return back->fLastMask.texture();
    }

    void acquireMask(int32_t clipGenID,
                     const GrTextureDesc& desc,
                     const SkIRect& bound) {

        if (fStack.empty()) {
            SkASSERT(false);
            return;
        }

        GrClipStackFrame* back = (GrClipStackFrame*) fStack.back();

        back->acquireMask(fContext, clipGenID, desc, bound);
    }

    int getLastMaskWidth() const {

        if (fStack.empty()) {
            SkASSERT(false);
            return -1;
        }

        GrClipStackFrame* back = (GrClipStackFrame*) fStack.back();

        if (NULL == back->fLastMask.texture()) {
            return -1;
        }

        return back->fLastMask.texture()->width();
    }

    int getLastMaskHeight() const {

        if (fStack.empty()) {
            SkASSERT(false);
            return -1;
        }

        GrClipStackFrame* back = (GrClipStackFrame*) fStack.back();

        if (NULL == back->fLastMask.texture()) {
            return -1;
        }

        return back->fLastMask.texture()->height();
    }

    void getLastBound(SkIRect* bound) const {

        if (fStack.empty()) {
            SkASSERT(false);
            bound->setEmpty();
            return;
        }

        GrClipStackFrame* back = (GrClipStackFrame*) fStack.back();

        *bound = back->fLastBound;
    }

    void setContext(GrContext* context) {
        fContext = context;
    }

    GrContext* getContext() {
        return fContext;
    }

    void releaseResources() {

        SkDeque::F2BIter iter(fStack);
        for (GrClipStackFrame* frame = (GrClipStackFrame*) iter.next();
                frame != NULL;
                frame = (GrClipStackFrame*) iter.next()) {
            frame->reset();
        }
    }

private:
    struct GrClipStackFrame {

        GrClipStackFrame() {
            this->reset();
        }

        void acquireMask(GrContext* context,
                         int32_t clipGenID,
                         const GrTextureDesc& desc,
                         const SkIRect& bound) {

            fLastClipGenID = clipGenID;

            fLastMask.set(context, desc);

            fLastBound = bound;
        }

        void reset () {
            fLastClipGenID = SkClipStack::kInvalidGenID;

            GrTextureDesc desc;

            fLastMask.set(NULL, desc);
            fLastBound.setEmpty();
        }

        int32_t                 fLastClipGenID;
        
        
        GrAutoScratchTexture    fLastMask;
        
        
        SkIRect                 fLastBound;
    };

    GrContext*   fContext;
    SkDeque      fStack;

    typedef SkNoncopyable INHERITED;
};

#endif 
