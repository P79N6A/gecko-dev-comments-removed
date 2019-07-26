








#ifndef SkPDFDocument_DEFINED
#define SkPDFDocument_DEFINED

#include "SkAdvancedTypefaceMetrics.h"
#include "SkRefCnt.h"
#include "SkTDArray.h"
#include "SkTScopedPtr.h"

class SkPDFCatalog;
class SkPDFDevice;
class SkPDFDict;
class SkPDFPage;
class SkPDFObject;
class SkWStream;





class SkPDFDocument {
public:
    enum Flags {
        kNoCompression_Flags = 0x01,  
        kNoLinks_Flags       = 0x02,  

        kDraftMode_Flags     = 0x01,
    };
    

    explicit SK_API SkPDFDocument(Flags flags = (Flags)0);
    SK_API ~SkPDFDocument();

    






    SK_API bool emitPDF(SkWStream* stream);

    






    SK_API bool setPage(int pageNumber, SkPDFDevice* pdfDevice);

    




    SK_API bool appendPage(SkPDFDevice* pdfDevice);

    

    SK_API void getCountOfFontTypes(
        int counts[SkAdvancedTypefaceMetrics::kNotEmbeddable_Font + 1]) const;

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
