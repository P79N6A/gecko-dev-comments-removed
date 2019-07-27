






#ifndef SkPicturePlayback_DEFINED
#define SkPicturePlayback_DEFINED

#include "SkPictureFlat.h"  
#include "SkPictureStateTree.h"

class SkBitmap;
class SkCanvas;
class SkDrawPictureCallback;
class SkPaint;
class SkPictureData;




class SkPicturePlayback : SkNoncopyable {
public:
    SkPicturePlayback(const SkPicture* picture)
        : fPictureData(picture->fData.get())
        , fCurOffset(0)
        , fUseBBH(true) {
    }
    virtual ~SkPicturePlayback() { }

    virtual void draw(SkCanvas* canvas, SkDrawPictureCallback*);

    
    
    
    size_t curOpID() const { return fCurOffset; }
    void resetOpID() { fCurOffset = 0; }

    
    void setUseBBH(bool useBBH) { fUseBBH = useBBH; }

protected:
    const SkPictureData* fPictureData;

    
    size_t fCurOffset;

    bool   fUseBBH;

    void handleOp(SkReader32* reader, 
                  DrawType op, 
                  uint32_t size, 
                  SkCanvas* canvas,
                  const SkMatrix& initialMatrix);

    const SkPicture::OperationList* getActiveOps(const SkCanvas* canvas);
    bool initIterator(SkPictureStateTree::Iterator* iter, 
                      SkCanvas* canvas,
                      const SkPicture::OperationList *activeOpsList);
    static void StepIterator(SkPictureStateTree::Iterator* iter, SkReader32* reader);
    static void SkipIterTo(SkPictureStateTree::Iterator* iter, 
                           SkReader32* reader, uint32_t skipTo);

    static DrawType ReadOpAndSize(SkReader32* reader, uint32_t* size);

    class AutoResetOpID {
    public:
        AutoResetOpID(SkPicturePlayback* playback) : fPlayback(playback) { }
        ~AutoResetOpID() {
            if (NULL != fPlayback) {
                fPlayback->resetOpID();
            }
        }

    private:
        SkPicturePlayback* fPlayback;
    };

private:
    typedef SkNoncopyable INHERITED;
};

#endif
