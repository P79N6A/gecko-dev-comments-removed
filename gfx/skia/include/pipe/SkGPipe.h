









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
    SkGPipeReader();
    SkGPipeReader(SkCanvas* target);
    ~SkGPipeReader();

    enum Status {
        kDone_Status,   
        kEOF_Status,    
        kError_Status,  
        kReadAtom_Status
    };

    void setCanvas(SkCanvas*);
    
    
    Status playback(const void* data, size_t length, size_t* bytesRead = NULL,
                    bool readAtom = false);
private:
    SkCanvas*           fCanvas;
    class SkGPipeState* fState;
};



class SkGPipeCanvas;

class SkGPipeController {
public:
    SkGPipeController() : fCanvas(NULL) {}
    virtual ~SkGPipeController();

    









    virtual void* requestBlock(size_t minRequest, size_t* actual) = 0;

    






    virtual void notifyWritten(size_t bytes) = 0;
    virtual int numberOfReaders() const { return 1; }

private:
    friend class SkGPipeWriter;
    void setCanvas(SkGPipeCanvas*);

    SkGPipeCanvas* fCanvas;
};

class SkGPipeWriter {
public:
    SkGPipeWriter();
    ~SkGPipeWriter();

    bool isRecording() const { return NULL != fCanvas; }

    enum Flags {
        



        kCrossProcess_Flag              = 1 << 0,

        




        kSharedAddressSpace_Flag        = 1 << 1,

        



        kSimultaneousReaders_Flag       = 1 << 2,
    };

    SkCanvas* startRecording(SkGPipeController*, uint32_t flags = 0,
        uint32_t width = kDefaultRecordingCanvasSize,
        uint32_t height = kDefaultRecordingCanvasSize);

    
    
    void endRecording();

    





    void flushRecording(bool detachCurrentBlock);

    






    size_t storageAllocatedForRecording() const;

    






    size_t freeMemoryIfPossible(size_t bytesToFree);

private:
    enum {
        kDefaultRecordingCanvasSize = 32767,
    };

    SkGPipeCanvas* fCanvas;
    SkWriter32     fWriter;
};

#endif
