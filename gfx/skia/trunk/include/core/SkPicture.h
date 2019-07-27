








#ifndef SkPicture_DEFINED
#define SkPicture_DEFINED

#include "SkBitmap.h"
#include "SkDrawPictureCallback.h"
#include "SkImageDecoder.h"
#include "SkRefCnt.h"
#include "SkTDArray.h"

#if SK_SUPPORT_GPU
class GrContext;
#endif

class SkBBHFactory;
class SkBBoxHierarchy;
class SkCanvas;
class SkData;
class SkPictureData;
class SkPictureRecord;
class SkStream;
class SkWStream;

struct SkPictInfo;

class SkRecord;






class SK_API SkPicture : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkPicture)

    
    
    
    class AccelData : public SkRefCnt {
    public:
        typedef uint8_t Domain;
        typedef uint32_t Key;

        AccelData(Key key) : fKey(key) { }

        const Key& getKey() const { return fKey; }

        
        
        static Domain GenerateDomain();
    private:
        Key fKey;

        typedef SkRefCnt INHERITED;
    };

#ifdef SK_SUPPORT_LEGACY_DEFAULT_PICTURE_CTOR
    SkPicture();
#endif

    
    void EXPERIMENTAL_addAccelData(const AccelData*) const;

    
    const AccelData* EXPERIMENTAL_getAccelData(AccelData::Key) const;

    










    typedef bool (*InstallPixelRefProc)(const void* src, size_t length, SkBitmap* dst);

    







    static SkPicture* CreateFromStream(SkStream*,
                                       InstallPixelRefProc proc = &SkImageDecoder::DecodeMemory);

    







    static SkPicture* CreateFromBuffer(SkReadBuffer&);

    virtual ~SkPicture();

#ifdef SK_SUPPORT_LEGACY_PICTURE_CLONE
    


    SkPicture* clone() const;
#endif

    


    void draw(SkCanvas* canvas, SkDrawPictureCallback* = NULL) const;

    




    int width() const { return fWidth; }

    




    int height() const { return fHeight; }

    




    uint32_t uniqueID() const;

    









    typedef SkData* (*EncodeBitmap)(size_t* pixelRefOffset, const SkBitmap& bm);

    




    void serialize(SkWStream*, EncodeBitmap encoder = NULL) const;

    


    void flatten(SkWriteBuffer&) const;

    



    bool willPlayBackBitmaps() const;

    







    static bool InternalOnly_StreamIsSKP(SkStream*, SkPictInfo*);
    static bool InternalOnly_BufferIsSKP(SkReadBuffer&, SkPictInfo*);

    


#if SK_SUPPORT_GPU
    bool suitableForGpuRasterization(GrContext*, const char ** = NULL) const;
#endif

    class DeletionListener : public SkRefCnt {
    public:
        virtual void onDeletion(uint32_t pictureID) = 0;
    };

    
    void addDeletionListener(DeletionListener* listener) const;

private:
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    

    
    static const uint32_t MIN_PICTURE_VERSION = 19;
    static const uint32_t CURRENT_PICTURE_VERSION = 30;

    mutable uint32_t      fUniqueID;

    
    SkAutoTDelete<SkPictureData> fData;
    int                   fWidth, fHeight;
    mutable SkAutoTUnref<const AccelData> fAccelData;

    mutable SkTDArray<DeletionListener*> fDeletionListeners;  

    void needsNewGenID() { fUniqueID = SK_InvalidGenID; }
    void callDeletionListeners();

    
    
    SkPicture(SkPictureData* data, int width, int height);

    SkPicture(int width, int height, const SkPictureRecord& record, bool deepCopyOps);

    
    
    class OperationList : ::SkNoncopyable {
    public:
        
        
        int numOps() const { return fOps.count(); }
        
        uint32_t offset(int index) const;
        
        const SkMatrix& matrix(int index) const;

        SkTDArray<void*> fOps;
    };

    


    const OperationList* EXPERIMENTAL_getActiveOps(const SkIRect& queryRect) const;

    void createHeader(SkPictInfo* info) const;
    static bool IsValidPictInfo(const SkPictInfo& info);

    friend class SkPictureData;                
    friend class SkPictureRecorder;            
    friend class SkGpuDevice;                  
    friend class GrGatherCanvas;               
    friend class SkPicturePlayback;            
    friend class SkPictureReplacementPlayback; 

    typedef SkRefCnt INHERITED;

    SkPicture(int width, int height, SkRecord*);  
    SkAutoTDelete<SkRecord> fRecord;
    bool fRecordWillPlayBackBitmaps; 
};

#endif
