








#ifndef SkPDFDocument_DEFINED
#define SkPDFDocument_DEFINED

#include "SkPDFTypes.h"
#include "SkRefCnt.h"
#include "SkTDArray.h"
#include "SkTScopedPtr.h"

class SkPDFCatalog;
class SkPDFDevice;
class SkPDFPage;
class SkWSteam;





class SkPDFDocument {
public:
    enum Flags {
        kNoCompression_Flag = 0x01,  
        kNoEmbedding_Flag   = 0x02,  

        kDraftMode_Flags    = 0x03,
    };
    

    explicit SK_API SkPDFDocument(Flags flags = (Flags)0);
    SK_API ~SkPDFDocument();

    






    SK_API bool emitPDF(SkWStream* stream);

    






    SK_API bool setPage(int pageNumber, SkPDFDevice* pdfDevice);

    




    SK_API bool appendPage(SkPDFDevice* pdfDevice);

    

    SK_API const SkTDArray<SkPDFPage*>& getPages();

private:
    SkTScopedPtr<SkPDFCatalog> fCatalog;
    int64_t fXRefFileOffset;

    SkTDArray<SkPDFPage*> fPages;
    SkTDArray<SkPDFDict*> fPageTree;
    SkRefPtr<SkPDFDict> fDocCatalog;
    SkTDArray<SkPDFObject*> fPageResources;
    SkTDArray<SkPDFObject*> fSubstitutes;
    int fSecondPageFirstResourceIndex;

    SkRefPtr<SkPDFDict> fTrailerDict;

    


    void emitHeader(SkWStream* stream);

    

    size_t headerSize();

    



    void emitFooter(SkWStream* stream, int64_t objCount);
};

#endif
