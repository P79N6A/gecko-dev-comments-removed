








#ifndef SkFlattenable_DEFINED
#define SkFlattenable_DEFINED

#include "SkRefCnt.h"
#include "SkBitmap.h"
#include "SkReader32.h"
#include "SkTDArray.h"
#include "SkWriter32.h"

class SkFlattenableReadBuffer;
class SkFlattenableWriteBuffer;
class SkString;

#if SK_ALLOW_STATIC_GLOBAL_INITIALIZERS

#define SK_DECLARE_FLATTENABLE_REGISTRAR() 

#define SK_DEFINE_FLATTENABLE_REGISTRAR(flattenable) \
    static SkFlattenable::Registrar g##flattenable##Reg(#flattenable, \
                                                      flattenable::CreateProc);
                                                      
#define SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_START(flattenable)
#define SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(flattenable) \
    static SkFlattenable::Registrar g##flattenable##Reg(#flattenable, \
                                                      flattenable::CreateProc);
#define SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_END

#else

#define SK_DECLARE_FLATTENABLE_REGISTRAR() static void Init();

#define SK_DEFINE_FLATTENABLE_REGISTRAR(flattenable) \
    void flattenable::Init() { \
        SkFlattenable::Registrar(#flattenable, CreateProc); \
    }

#define SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_START(flattenable) \
    void flattenable::Init() {
    
#define SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(flattenable) \
        SkFlattenable::Registrar(#flattenable, flattenable::CreateProc);
    
#define SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_END \
    }

#endif







class SK_API SkFlattenable : public SkRefCnt {
public:
    typedef SkFlattenable* (*Factory)(SkFlattenableReadBuffer&);
    
    SkFlattenable() {}
    
    



    virtual Factory getFactory() = 0;
    



    virtual void flatten(SkFlattenableWriteBuffer&);
    
    


    virtual bool toDumpString(SkString*) const;

    static Factory NameToFactory(const char name[]);
    static const char* FactoryToName(Factory);
    static void Register(const char name[], Factory);
    
    class Registrar {
    public:
        Registrar(const char name[], Factory factory) {
            SkFlattenable::Register(name, factory);
        }
    };
    
protected:
    SkFlattenable(SkFlattenableReadBuffer&) {}

private:
#if !SK_ALLOW_STATIC_GLOBAL_INITIALIZERS
    static void InitializeFlattenables();
#endif

    friend class SkGraphics;
};



class SkMatrix;
extern void SkReadMatrix(SkReader32*, SkMatrix*);
extern void SkWriteMatrix(SkWriter32*, const SkMatrix&);

class SkRegion;
extern void SkReadRegion(SkReader32*, SkRegion*);
extern void SkWriteRegion(SkWriter32*, const SkRegion&);




class SkTypeface;

class SkFlattenableReadBuffer : public SkReader32 {
public:
    SkFlattenableReadBuffer();
    explicit SkFlattenableReadBuffer(const void* data);
    SkFlattenableReadBuffer(const void* data, size_t size);
    
    void setRefCntArray(SkRefCnt* array[], int count) {
        fRCArray = array;
        fRCCount = count;
    }
    
    void setTypefaceArray(SkTypeface* array[], int count) {
        fTFArray = array;
        fTFCount = count;
    }

    



    void setFactoryPlayback(SkFlattenable::Factory array[], int count) {
        fFactoryTDArray = NULL;
        fFactoryArray = array;
        fFactoryCount = count;
    }

    




    void setFactoryArray(SkTDArray<SkFlattenable::Factory>* array) {
        fFactoryTDArray = array;
        fFactoryArray = NULL;
        fFactoryCount = 0;
    }
    
    SkTypeface* readTypeface();
    SkRefCnt* readRefCnt();
    void* readFunctionPtr();
    SkFlattenable* readFlattenable();
    
private:
    SkRefCnt** fRCArray;
    int        fRCCount;
    
    SkTypeface** fTFArray;
    int        fTFCount;
    
    SkTDArray<SkFlattenable::Factory>* fFactoryTDArray;
    SkFlattenable::Factory* fFactoryArray;
    int                     fFactoryCount;
    
    typedef SkReader32 INHERITED;
};



#include "SkPtrRecorder.h"






class SkRefCntSet : public SkTPtrSet<SkRefCnt*> {
public:
    virtual ~SkRefCntSet();
    
protected:
    
    virtual void incPtr(void*);
    virtual void decPtr(void*);
};

class SkFactorySet : public SkTPtrSet<SkFlattenable::Factory> {};

class SkFlattenableWriteBuffer : public SkWriter32 {
public:
    SkFlattenableWriteBuffer(size_t minSize);
    virtual ~SkFlattenableWriteBuffer();

    void writeTypeface(SkTypeface*);
    void writeRefCnt(SkRefCnt*);
    void writeFunctionPtr(void*);
    void writeFlattenable(SkFlattenable* flattenable);
    
    SkRefCntSet* getTypefaceRecorder() const { return fTFSet; }
    SkRefCntSet* setTypefaceRecorder(SkRefCntSet*);
    
    SkRefCntSet* getRefCntRecorder() const { return fRCSet; }
    SkRefCntSet* setRefCntRecorder(SkRefCntSet*);
    
    SkFactorySet* getFactoryRecorder() const { return fFactorySet; }
    SkFactorySet* setFactoryRecorder(SkFactorySet*);

    enum Flags {
        kCrossProcess_Flag       = 0x01,
        



        kInlineFactoryNames_Flag = 0x02
    };
    Flags getFlags() const { return (Flags)fFlags; }
    void setFlags(Flags flags) { fFlags = flags; }
    
    bool isCrossProcess() const {
        return SkToBool(fFlags & kCrossProcess_Flag);
    }
    bool inlineFactoryNames() const {
        return SkToBool(fFlags & kInlineFactoryNames_Flag);
    }

    bool persistBitmapPixels() const {
        return (fFlags & kCrossProcess_Flag) != 0;
    }
    
    bool persistTypeface() const { return (fFlags & kCrossProcess_Flag) != 0; }

private:
    uint32_t        fFlags;
    SkRefCntSet*    fTFSet;
    SkRefCntSet*    fRCSet;
    SkFactorySet*   fFactorySet;
    
    typedef SkWriter32 INHERITED;
};

#endif

