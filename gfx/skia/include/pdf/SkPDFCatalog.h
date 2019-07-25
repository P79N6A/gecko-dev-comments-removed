








#ifndef SkPDFCatalog_DEFINED
#define SkPDFCatalog_DEFINED

#include <sys/types.h>

#include "SkPDFDocument.h"
#include "SkPDFTypes.h"
#include "SkRefCnt.h"
#include "SkTDArray.h"






class SK_API SkPDFCatalog {
public:
    

    explicit SkPDFCatalog(SkPDFDocument::Flags flags);
    ~SkPDFCatalog();

    




    SkPDFObject* addObject(SkPDFObject* obj, bool onFirstPage);

    





    size_t setFileOffset(SkPDFObject* obj, size_t offset);

    



    void emitObjectNumber(SkWStream* stream, SkPDFObject* obj);

    



    size_t getObjectNumberSize(SkPDFObject* obj);

    

    SkPDFDocument::Flags getDocumentFlags() const { return fDocumentFlags; }

    





    int32_t emitXrefTable(SkWStream* stream, bool firstPage);

    

    void setSubstitute(SkPDFObject* original, SkPDFObject* substitute);

    


    SkPDFObject* getSubstituteObject(SkPDFObject* object);

    




    off_t setSubstituteResourcesOffsets(off_t fileOffset, bool firstPage);

    

    void emitSubstituteResources(SkWStream* stream, bool firstPage);

private:
    struct Rec {
        Rec(SkPDFObject* object, bool onFirstPage)
            : fObject(object),
              fFileOffset(0),
              fObjNumAssigned(false),
              fOnFirstPage(onFirstPage) {
        }
        SkPDFObject* fObject;
        off_t fFileOffset;
        bool fObjNumAssigned;
        bool fOnFirstPage;
    };

    struct SubstituteMapping {
        SubstituteMapping(SkPDFObject* original, SkPDFObject* substitute)
            : fOriginal(original), fSubstitute(substitute) {
        }
        SkPDFObject* fOriginal;
        SkPDFObject* fSubstitute;
    };

    
    SkTDArray<struct Rec> fCatalog;

    
    SkTDArray<SubstituteMapping> fSubstituteMap;
    SkTDArray<SkPDFObject*> fSubstituteResourcesFirstPage;
    SkTDArray<SkPDFObject*> fSubstituteResourcesRemaining;

    
    uint32_t fFirstPageCount;
    
    uint32_t fNextObjNum;
    
    uint32_t fNextFirstPageObjNum;

    SkPDFDocument::Flags fDocumentFlags;

    int findObjectIndex(SkPDFObject* obj) const;

    int assignObjNum(SkPDFObject* obj);

    SkTDArray<SkPDFObject*>* getSubstituteList(bool firstPage);
};

#endif
