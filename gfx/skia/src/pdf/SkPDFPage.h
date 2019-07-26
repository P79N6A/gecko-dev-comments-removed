








#ifndef SkPDFPage_DEFINED
#define SkPDFPage_DEFINED

#include "SkPDFTypes.h"
#include "SkPDFStream.h"
#include "SkRefCnt.h"
#include "SkTDArray.h"

class SkPDFCatalog;
class SkPDFDevice;
class SkWStream;






class SkPDFPage : public SkPDFDict {
public:
    



    explicit SkPDFPage(SkPDFDevice* content);
    ~SkPDFPage();

    












    void finalizePage(SkPDFCatalog* catalog, bool firstPage,
                      const SkTSet<SkPDFObject*>& knownResourceObjects,
                      SkTSet<SkPDFObject*>* newResourceObjects);

    


    void appendDestinations(SkPDFDict* dict);

    






    off_t getPageSize(SkPDFCatalog* catalog, off_t fileOffset);

    



    void emitPage(SkWStream* stream, SkPDFCatalog* catalog);

    











    static void GeneratePageTree(const SkTDArray<SkPDFPage*>& pages,
                                 SkPDFCatalog* catalog,
                                 SkTDArray<SkPDFDict*>* pageTree,
                                 SkPDFDict** rootNode);

    

    const SkTDArray<SkPDFFont*>& getFontResources() const;

    


    const SkPDFGlyphSetMap& getFontGlyphUsage() const;

private:
    
    SkAutoTUnref<SkPDFDevice> fDevice;

    
    SkAutoTUnref<SkPDFStream> fContentStream;
};

#endif
