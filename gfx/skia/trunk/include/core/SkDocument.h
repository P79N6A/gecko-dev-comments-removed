






#ifndef SkDocument_DEFINED
#define SkDocument_DEFINED

#include "SkBitmap.h"
#include "SkPicture.h"
#include "SkRect.h"
#include "SkRefCnt.h"

class SkCanvas;
class SkWStream;



#define SK_ScalarDefaultRasterDPI           72.0f











class SkDocument : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkDocument)

    













    static SkDocument* CreatePDF(
            const char filename[],
            SkPicture::EncodeBitmap encoder = NULL,
            SkScalar rasterDpi = SK_ScalarDefaultRasterDPI);

    





















    static SkDocument* CreatePDF(
            SkWStream*, void (*Done)(SkWStream*,bool aborted) = NULL,
            SkPicture::EncodeBitmap encoder = NULL,
            SkScalar rasterDpi = SK_ScalarDefaultRasterDPI);

    




    SkCanvas* beginPage(SkScalar width, SkScalar height,
                        const SkRect* content = NULL);

    




    void endPage();

    






    bool close();

    



    void abort();

protected:
    SkDocument(SkWStream*, void (*)(SkWStream*, bool aborted));
    
    
    virtual ~SkDocument();

    virtual SkCanvas* onBeginPage(SkScalar width, SkScalar height,
                                  const SkRect& content) = 0;
    virtual void onEndPage() = 0;
    virtual bool onClose(SkWStream*) = 0;
    virtual void onAbort() = 0;

    enum State {
        kBetweenPages_State,
        kInPage_State,
        kClosed_State
    };
    State getState() const { return fState; }

private:
    SkWStream* fStream;
    void       (*fDoneProc)(SkWStream*, bool aborted);
    State      fState;

    typedef SkRefCnt INHERITED;
};

#endif
