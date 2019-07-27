






#ifndef SkSurface_DEFINED
#define SkSurface_DEFINED

#include "SkRefCnt.h"
#include "SkImage.h"

class SkCanvas;
class SkPaint;
class GrContext;
class GrRenderTarget;









class SK_API SkSurface : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkSurface)

    






    static SkSurface* NewRasterDirect(const SkImageInfo&, void* pixels, size_t rowBytes);

    



    static SkSurface* NewRasterDirectReleaseProc(const SkImageInfo&, void* pixels, size_t rowBytes,
                                                 void (*releaseProc)(void* pixels, void* context),
                                                 void* context);

    






    static SkSurface* NewRaster(const SkImageInfo&);

    




    static SkSurface* NewRasterPMColor(int width, int height) {
        return NewRaster(SkImageInfo::MakeN32Premul(width, height));
    }

    


    enum TextRenderMode {
        


        kStandard_TextRenderMode,
        


        kDistanceField_TextRenderMode,
    };

    enum RenderTargetFlags {
        kNone_RenderTargetFlag      = 0x0,
        



        kDontClear_RenderTargetFlag = 0x01,
    };

    


    static SkSurface* NewRenderTargetDirect(GrRenderTarget*,
                                            TextRenderMode trm = kStandard_TextRenderMode,
                                            RenderTargetFlags flags = kNone_RenderTargetFlag);

    



    static SkSurface* NewRenderTarget(GrContext*, const SkImageInfo&, int sampleCount = 0,
                                      TextRenderMode trm = kStandard_TextRenderMode,
                                      RenderTargetFlags flags = kNone_RenderTargetFlag);

    











    static SkSurface* NewScratchRenderTarget(GrContext*, const SkImageInfo&, int sampleCount = 0,
                                             TextRenderMode trm = kStandard_TextRenderMode,
                                             RenderTargetFlags flags = kNone_RenderTargetFlag);

    int width() const { return fWidth; }
    int height() const { return fHeight; }

    








    uint32_t generationID();

    


    enum ContentChangeMode {
        



        kDiscard_ContentChangeMode,
        



        kRetain_ContentChangeMode,
    };

    



    void notifyContentWillChange(ContentChangeMode mode);

    





    SkCanvas* getCanvas();

    












    SkSurface* newSurface(const SkImageInfo&);

    




    SkImage* newImageSnapshot();

    






    void draw(SkCanvas*, SkScalar x, SkScalar y, const SkPaint*);

    









    const void* peekPixels(SkImageInfo* info, size_t* rowBytes);

protected:
    SkSurface(int width, int height);
    SkSurface(const SkImageInfo&);

    
    void dirtyGenerationID() {
        fGenerationID = 0;
    }

private:
    const int   fWidth;
    const int   fHeight;
    uint32_t    fGenerationID;

    typedef SkRefCnt INHERITED;
};

#endif
