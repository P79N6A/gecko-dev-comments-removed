








#ifndef SkPDFTypes_DEFINED
#define SkPDFTypes_DEFINED

#include "SkRefCnt.h"
#include "SkScalar.h"
#include "SkString.h"
#include "SkTDArray.h"
#include "SkTypes.h"

class SkPDFCatalog;
class SkWStream;







class SkPDFObject : public SkRefCnt {
public:
    

    SkPDFObject();
    virtual ~SkPDFObject();

    





    virtual size_t getOutputSize(SkPDFCatalog* catalog, bool indirect);

    






    virtual void getResources(SkTDArray<SkPDFObject*>* resourceList);

    



    void emit(SkWStream* stream, SkPDFCatalog* catalog, bool indirect);

    



    void emitIndirectObject(SkWStream* stream, SkPDFCatalog* catalog);

    


    size_t getIndirectOutputSize(SkPDFCatalog* catalog);

    




    static void AddResourceHelper(SkPDFObject* resource,
                                  SkTDArray<SkPDFObject*>* list);

    




    static void GetResourcesHelper(SkTDArray<SkPDFObject*>* resources,
                                   SkTDArray<SkPDFObject*>* result);

protected:
    





    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect) = 0;
};





class SkPDFObjRef : public SkPDFObject {
public:
    


    explicit SkPDFObjRef(SkPDFObject* obj);
    virtual ~SkPDFObjRef();

    
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);
    virtual size_t getOutputSize(SkPDFCatalog* catalog, bool indirect);

private:
    SkRefPtr<SkPDFObject> fObj;
};





class SkPDFInt : public SkPDFObject {
public:
    


    explicit SkPDFInt(int32_t value);
    virtual ~SkPDFInt();

    
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);

private:
    int32_t fValue;
};





class SkPDFBool : public SkPDFObject {
public:
    


    explicit SkPDFBool(bool value);
    virtual ~SkPDFBool();

    
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);
    virtual size_t getOutputSize(SkPDFCatalog* catalog, bool indirect);

private:
    bool fValue;
};





class SkPDFScalar : public SkPDFObject {
public:
    


    explicit SkPDFScalar(SkScalar value);
    virtual ~SkPDFScalar();

    static void Append(SkScalar value, SkWStream* stream);

    
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);

private:
    SkScalar fValue;
};





class SkPDFString : public SkPDFObject {
public:
    


    explicit SkPDFString(const char value[]);
    explicit SkPDFString(const SkString& value);

    





    SkPDFString(const uint16_t* value, size_t len, bool wideChars);
    virtual ~SkPDFString();

    
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);
    virtual size_t getOutputSize(SkPDFCatalog* catalog, bool indirect);

    static SkString FormatString(const char* input, size_t len);
    static SkString FormatString(const uint16_t* input, size_t len,
                                 bool wideChars);
private:
    static const size_t kMaxLen = 65535;

    const SkString fValue;

    static SkString DoFormatString(const void* input, size_t len,
                                 bool wideInput, bool wideOutput);
};





class SkPDFName : public SkPDFObject {
public:
    


    explicit SkPDFName(const char name[]);
    explicit SkPDFName(const SkString& name);
    virtual ~SkPDFName();

    bool operator==(const SkPDFName& b) const;

    
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);
    virtual size_t getOutputSize(SkPDFCatalog* catalog, bool indirect);

private:
    static const size_t kMaxLen = 127;

    const SkString fValue;

    static SkString FormatName(const SkString& input);
};





class SkPDFArray : public SkPDFObject {
public:
    

    SkPDFArray();
    virtual ~SkPDFArray();

    
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);
    virtual size_t getOutputSize(SkPDFCatalog* catalog, bool indirect);

    

    int size() { return fValue.count(); }

    


    void reserve(int length);

    


    SkPDFObject* getAt(int index) { return fValue[index]; }

    




    SkPDFObject* setAt(int index, SkPDFObject* value);

    



    SkPDFObject* append(SkPDFObject* value);

    


    void appendInt(int32_t value);

    


    void appendScalar(SkScalar value);

    


    void appendName(const char name[]);

private:
    static const int kMaxLen = 8191;
    SkTDArray<SkPDFObject*> fValue;
};





class SkPDFDict : public SkPDFObject {
public:
    

    SkPDFDict();

    


    explicit SkPDFDict(const char type[]);

    virtual ~SkPDFDict();

    
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);
    virtual size_t getOutputSize(SkPDFCatalog* catalog, bool indirect);

    

    int size() { return fValue.count(); }

    




    SkPDFObject* insert(SkPDFName* key, SkPDFObject* value);

    





    SkPDFObject* insert(const char key[], SkPDFObject* value);

    



    void insertInt(const char key[], int32_t value);

    



    void insertScalar(const char key[], SkScalar value);

    



    void insertName(const char key[], const char name[]);

    



    void insertName(const char key[], const SkString& name) {
        this->insertName(key, name.c_str());
    }

    

    void clear();

private:
    struct Rec {
      SkPDFName* key;
      SkPDFObject* value;
    };

public:
    class Iter {
    public:
        explicit Iter(const SkPDFDict& dict);
        SkPDFName* next(SkPDFObject** value);

    private:
        Rec* fIter;
        Rec* fStop;
    };

private:
    static const int kMaxLen = 4095;

    SkTDArray<struct Rec> fValue;
};

#endif
