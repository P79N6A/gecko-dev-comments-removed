








#ifndef SkPDFStream_DEFINED
#define SkPDFStream_DEFINED

#include "SkPDFTypes.h"
#include "SkRefCnt.h"
#include "SkStream.h"
#include "SkTemplates.h"

class SkPDFCatalog;






class SkPDFStream : public SkPDFDict {
    SK_DECLARE_INST_COUNT(SkPDFStream)
public:
    



    explicit SkPDFStream(SkData* data);

    



    explicit SkPDFStream(SkStream* stream);

    virtual ~SkPDFStream();

    
    
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);
    virtual size_t getOutputSize(SkPDFCatalog* catalog, bool indirect);

protected:
    enum State {
        kUnused_State,         
        kNoCompression_State,  
                               
        kCompressed_State,     
    };

    


    explicit SkPDFStream(const SkPDFStream& pdfStream);

    


    SkPDFStream();

    
    
    virtual bool populate(SkPDFCatalog* catalog);

    void setSubstitute(SkPDFStream* stream) {
        fSubstitute.reset(stream);
    }

    SkPDFStream* getSubstitute() const {
        return fSubstitute.get();
    }

    void setData(SkData* data);
    void setData(SkStream* stream);

    size_t dataSize() const;

    void setState(State state) {
        fState = state;
    }

    State getState() const {
        return fState;
    }

private:
    
    State fState;

    
    SkMutex fMutex;

    SkMemoryStream fMemoryStream;  
                                   
                                   
    SkAutoTUnref<SkStreamRewindable> fDataStream;
    SkAutoTUnref<SkPDFStream> fSubstitute;

    typedef SkPDFDict INHERITED;
};

#endif
