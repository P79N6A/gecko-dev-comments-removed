









#ifndef SkGPipe_DEFINED
#define SkGPipe_DEFINED

#include "SkWriter32.h"
#include "SkFlattenable.h"

class SkCanvas;


#ifdef Status
    #undef Status
#endif

class SkGPipeReader {
public:
    SkGPipeReader(SkCanvas* target);
    ~SkGPipeReader();

    enum Status {
        kDone_Status,   
        kEOF_Status,    
        kError_Status,  
        kReadAtom_Status
    };

    
    
    Status playback(const void* data, size_t length, size_t* bytesRead = NULL,
                    bool readAtom = false);
private:
    SkCanvas*           fCanvas;
    class SkGPipeState* fState;
};



class SkGPipeController {
public:
    









    virtual void* requestBlock(size_t minRequest, size_t* actual) = 0;

    






    virtual void notifyWritten(size_t bytes) = 0;
};

class SkGPipeWriter {
public:
    SkGPipeWriter();
    ~SkGPipeWriter();

    bool isRecording() const { return NULL != fCanvas; }

    enum Flags {
        kCrossProcess_Flag = 1 << 0,
    };

    SkCanvas* startRecording(SkGPipeController*, uint32_t flags = 0);

    
    
    void endRecording();

private:
    class SkGPipeCanvas* fCanvas;
    SkGPipeController*   fController;
    SkFactorySet         fFactorySet;
    SkWriter32 fWriter;
};

#endif
