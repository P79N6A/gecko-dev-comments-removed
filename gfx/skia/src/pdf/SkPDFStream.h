








#ifndef SkPDFStream_DEFINED
#define SkPDFStream_DEFINED

#include "SkPDFTypes.h"
#include "SkRefCnt.h"
#include "SkStream.h"
#include "SkTemplates.h"

class SkPDFCatalog;






class SkPDFStream : public SkPDFDict {
public:
    




    explicit SkPDFStream(SkData* data);
    
    explicit SkPDFStream(SkStream* stream);
    


    explicit SkPDFStream(const SkPDFStream& pdfStream);
    virtual ~SkPDFStream();

    
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);
    virtual size_t getOutputSize(SkPDFCatalog* catalog, bool indirect);

protected:
    


    SkPDFStream();

    void setData(SkStream* stream);

private:
    enum State {
        kUnused_State,         
        kNoCompression_State,  
                               
        kCompressed_State,     
    };
    
    State fState;

    
    SkRefPtr<SkStream> fData;
    SkRefPtr<SkPDFStream> fSubstitute;

    typedef SkPDFDict INHERITED;

    
    
    bool populate(SkPDFCatalog* catalog);
};

#endif
