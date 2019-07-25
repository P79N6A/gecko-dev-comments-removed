









#ifndef GrTextContext_DEFINED
#define GrTextContext_DEFINED

#include "GrGlyph.h"
#include "GrMatrix.h"
#include "GrRefCnt.h"

class GrContext;
class GrFontScaler;
class GrPaint;

class SkGpuDevice;
class SkPaint;






class GrTextContext: public GrRefCnt {
protected:
    GrContext*      fContext;

public:
    



    class AutoFinish {
    public:
        AutoFinish(GrTextContext* textContext, GrContext* context,
                   const GrPaint&, const GrMatrix* extMatrix);
        ~AutoFinish();
        GrTextContext* getTextContext() const;

    private:
        GrTextContext* fTextContext;
    };

    virtual void drawPackedGlyph(GrGlyph::PackedID, GrFixed left, GrFixed top,
                                 GrFontScaler*) = 0;

    virtual ~GrTextContext() {}

protected:
    GrTextContext() {
        fContext = NULL;
    }

    bool isValid() const {
        return (NULL != fContext);
    }

    








    virtual void init(GrContext* context, const GrPaint&,
                      const GrMatrix* extMatrix) {
        fContext = context;
    }

    









    virtual void finish() {
        fContext = NULL;
    }

private:
    typedef GrRefCnt INHERITED;
};

inline GrTextContext::AutoFinish::AutoFinish(GrTextContext* textContext,
                                             GrContext* context,
                                             const GrPaint& grPaint,
                                             const GrMatrix* extMatrix) {
    GrAssert(NULL != textContext);
    fTextContext = textContext;
    fTextContext->ref();
    fTextContext->init(context, grPaint, extMatrix);
}

inline GrTextContext::AutoFinish::~AutoFinish() {
    fTextContext->finish();
    fTextContext->unref();
}

inline GrTextContext* GrTextContext::AutoFinish::getTextContext() const {
    return fTextContext;
}

#endif
