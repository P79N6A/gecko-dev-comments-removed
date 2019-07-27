








#ifndef SkPDFDocument_DEFINED
#define SkPDFDocument_DEFINED

#include "SkAdvancedTypefaceMetrics.h"
#include "SkRefCnt.h"
#include "SkTDArray.h"
#include "SkTemplates.h"

class SkPDFCatalog;
class SkPDFDevice;
class SkPDFDict;
class SkPDFPage;
class SkPDFObject;
class SkWStream;
template <typename T> class SkTSet;





class SkPDFDocument {
public:
    enum Flags {
        kNoCompression_Flags = 0x01,  
        kFavorSpeedOverSize_Flags = 0x01,  
                                           
                                           
        kNoLinks_Flags       = 0x02,  

        kDraftMode_Flags     = 0x01,
    };
    

    explicit SK_API SkPDFDocument(Flags flags = (Flags)0);
    SK_API ~SkPDFDocument();

    






    SK_API bool emitPDF(SkWStream* stream);

    






    SK_API bool setPage(int pageNumber, SkPDFDevice* pdfDevice);

    




    SK_API bool appendPage(SkPDFDevice* pdfDevice);

    


    SK_API void getCountOfFontTypes(
        int counts[SkAdvancedTypefaceMetrics::kOther_Font + 2]) const;

    

    SK_API void getCountOfFontTypes(
        int counts[SkAdvancedTypefaceMetrics::kOther_Font + 1],
        int* notSubsettableCount,
        int* notEmbedddableCount) const;

private:
    SkAutoTDelete<SkPDFCatalog> fCatalog;
    int64_t fXRefFileOffset;

    SkTDArray<SkPDFPage*> fPages;
    SkTDArray<SkPDFDict*> fPageTree;
    SkPDFDict* fDocCatalog;
    SkTSet<SkPDFObject*>* fFirstPageResources;
    SkTSet<SkPDFObject*>* fOtherPageResources;
    SkTDArray<SkPDFObject*> fSubstitutes;

    SkPDFDict* fTrailerDict;

    


    void emitHeader(SkWStream* stream);

    

    size_t headerSize();

    



    void emitFooter(SkWStream* stream, int64_t objCount);
};

#endif
