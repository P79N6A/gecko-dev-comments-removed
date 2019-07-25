







#ifndef GrClipMaskManager_DEFINED
#define GrClipMaskManager_DEFINED

#include "GrRect.h"
#include "SkPath.h"
#include "GrNoncopyable.h"
#include "GrClip.h"
#include "SkRefCnt.h"
#include "GrTexture.h"
#include "SkDeque.h"
#include "GrContext.h"

class GrGpu;
class GrPathRenderer;
class GrPathRendererChain;
class SkPath;
class GrTexture;
class GrDrawState;







struct ScissoringSettings {
    bool    fEnableScissoring;
    GrIRect fScissorRect;

    void setupScissoring(GrGpu* gpu);
};





class GrClipMaskCache : public GrNoncopyable {
public:
    GrClipMaskCache() 
    : fContext(NULL)
    , fStack(sizeof(GrClipStackFrame)) {
        
        
        new (fStack.push_back()) GrClipStackFrame();
    }

    ~GrClipMaskCache() {

        while (!fStack.empty()) {
            GrClipStackFrame* temp = (GrClipStackFrame*) fStack.back();
            temp->~GrClipStackFrame();
            fStack.pop_back();
        }
    }

    bool canReuse(const GrClip& clip, int width, int height) {

        if (fStack.empty()) {
            GrAssert(false);
            return false;
        }

        GrClipStackFrame* back = (GrClipStackFrame*) fStack.back();

        if (back->fLastMask.texture() &&
            back->fLastMask.texture()->width() >= width &&
            back->fLastMask.texture()->height() >= height &&
            clip == back->fLastClip) {
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

    





    void push() {
        new (fStack.push_back()) GrClipStackFrame();
    }

    void pop() {
        

        if (!fStack.empty()) {
            GrClipStackFrame* back = (GrClipStackFrame*) fStack.back();

            back->~GrClipStackFrame();
            fStack.pop_back();
        }
    }

    void getLastClip(GrClip* clip) const {

        if (fStack.empty()) {
            GrAssert(false);
            clip->setEmpty();
            return;
        }

        GrClipStackFrame* back = (GrClipStackFrame*) fStack.back();

        *clip = back->fLastClip;
    }

    GrTexture* getLastMask() {

        if (fStack.empty()) {
            GrAssert(false);
            return NULL;
        }

        GrClipStackFrame* back = (GrClipStackFrame*) fStack.back();

        return back->fLastMask.texture();
    }

    const GrTexture* getLastMask() const {

        if (fStack.empty()) {
            GrAssert(false);
            return NULL;
        }

        GrClipStackFrame* back = (GrClipStackFrame*) fStack.back();

        return back->fLastMask.texture();
    }

    void acquireMask(const GrClip& clip,
                     const GrTextureDesc& desc,
                     const GrIRect& bound) {

        if (fStack.empty()) {
            GrAssert(false);
            return;
        }

        GrClipStackFrame* back = (GrClipStackFrame*) fStack.back();

        back->acquireMask(fContext, clip, desc, bound);
    }

    int getLastMaskWidth() const {

        if (fStack.empty()) {
            GrAssert(false);
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
            GrAssert(false);
            return -1;
        }

        GrClipStackFrame* back = (GrClipStackFrame*) fStack.back();

        if (NULL == back->fLastMask.texture()) {
            return -1;
        }

        return back->fLastMask.texture()->height();
    }

    void getLastBound(GrIRect* bound) const {

        if (fStack.empty()) {
            GrAssert(false);
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

protected:
private:
    struct GrClipStackFrame {

        GrClipStackFrame() {
            reset();
        }

        void acquireMask(GrContext* context,
                         const GrClip& clip, 
                         const GrTextureDesc& desc,
                         const GrIRect& bound) {

            fLastClip = clip;

            fLastMask.set(context, desc);

            fLastBound = bound;
        }

        void reset () {
            fLastClip.setEmpty();

            const GrTextureDesc desc = { kNone_GrTextureFlags, 0, 0, 
                                         kUnknown_GrPixelConfig, 0 };

            fLastMask.set(NULL, desc);
            fLastBound.setEmpty();
        }

        GrClip                  fLastClip;
        
        
        GrAutoScratchTexture    fLastMask;
        
        
        
        GrIRect                 fLastBound;
    };

    GrContext*   fContext;
    SkDeque      fStack;

    typedef GrNoncopyable INHERITED;
};









class GrClipMaskManager : public GrNoncopyable {
public:
    GrClipMaskManager()
        : fClipMaskInStencil(false)
        , fClipMaskInAlpha(false) {
    }

    bool createClipMask(GrGpu* gpu, 
                        const GrClip& clip, 
                        ScissoringSettings* scissorSettings);

    void releaseResources();

    bool isClipInStencil() const { return fClipMaskInStencil; }
    bool isClipInAlpha() const { return fClipMaskInAlpha; }

    void resetMask() {
        fClipMaskInStencil = false;
    }

    void postClipPush() {
        
        
        
        fAACache.push();
    }

    void preClipPop() {
        fAACache.pop();
    }

    void setContext(GrContext* context) {
        fAACache.setContext(context);
    }

    GrContext* getContext() {
        return fAACache.getContext();
    }

protected:
private:
    bool fClipMaskInStencil;        
    bool fClipMaskInAlpha;          
    GrClipMaskCache fAACache;       

    bool createStencilClipMask(GrGpu* gpu, 
                               const GrClip& clip, 
                               const GrRect& bounds,
                               ScissoringSettings* scissorSettings);
    bool createAlphaClipMask(GrGpu* gpu,
                             const GrClip& clipIn,
                             GrTexture** result,
                             GrIRect *resultBounds);
    bool createSoftwareClipMask(GrGpu* gpu,
                                const GrClip& clipIn,
                                GrTexture** result,
                                GrIRect *resultBounds);
    bool clipMaskPreamble(GrGpu* gpu,
                          const GrClip& clipIn,
                          GrTexture** result,
                          GrIRect *resultBounds);

    bool useSWOnlyPath(GrGpu* gpu, const GrClip& clipIn);

    bool drawClipShape(GrGpu* gpu,
                       GrTexture* target,
                       const GrClip& clipIn,
                       int index);

    void drawTexture(GrGpu* gpu,
                     GrTexture* target,
                     GrTexture* texture);

    void getTemp(const GrIRect& bounds, GrAutoScratchTexture* temp);

    void setupCache(const GrClip& clip, 
                    const GrIRect& bounds);

    typedef GrNoncopyable INHERITED;
};

#endif 
