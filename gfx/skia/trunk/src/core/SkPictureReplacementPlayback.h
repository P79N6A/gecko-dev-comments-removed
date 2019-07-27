






#ifndef SkPictureReplacementPlayback_DEFINED
#define SkPictureReplacementPlayback_DEFINED

#include "SkPicturePlayback.h"



class SkPictureReplacementPlayback : public SkPicturePlayback {
public:
    
    
    class PlaybackReplacements {
    public:
        
        
        
        struct ReplacementInfo {
            size_t          fStart;
            size_t          fStop;
            SkIPoint        fPos;
            SkBitmap*       fBM;     
            const SkPaint*  fPaint;  

            SkIRect         fSrcRect;
        };

        ~PlaybackReplacements() { this->freeAll(); }

        
        
        
        ReplacementInfo* push();

        
        ReplacementInfo* lookupByStart(size_t start);

    private:
        SkTDArray<ReplacementInfo> fReplacements;

        void freeAll();

#ifdef SK_DEBUG
        void validate() const;
#endif
    };

    
    
    
    
    
    SkPictureReplacementPlayback(const SkPicture* picture, 
                                 PlaybackReplacements* replacements,
                                 const SkPicture::OperationList* activeOpsList)
        : INHERITED(picture)
        , fReplacements(replacements)
        , fActiveOpsList(activeOpsList) {
    }

    virtual void draw(SkCanvas* canvas, SkDrawPictureCallback*) SK_OVERRIDE;

private:
    PlaybackReplacements*           fReplacements;
    const SkPicture::OperationList* fActiveOpsList;

    
    
    
    
    
    bool replaceOps(SkPictureStateTree::Iterator* iter,
                    SkReader32* reader,
                    SkCanvas* canvas,
                    const SkMatrix& initialMatrix);

    typedef SkPicturePlayback INHERITED;
};

#endif
