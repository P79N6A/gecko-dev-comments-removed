








#ifndef SkPDFTypes_DEFINED
#define SkPDFTypes_DEFINED

#include "SkRefCnt.h"
#include "SkScalar.h"
#include "SkString.h"
#include "SkTDArray.h"
#include "SkTSet.h"
#include "SkTypes.h"

class SkPDFCatalog;
class SkWStream;







class SkPDFObject : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkPDFObject)

    





    virtual size_t getOutputSize(SkPDFCatalog* catalog, bool indirect);

    








    virtual void getResources(const SkTSet<SkPDFObject*>& knownResourceObjects,
                              SkTSet<SkPDFObject*>* newResourceObjects);

    



    void emit(SkWStream* stream, SkPDFCatalog* catalog, bool indirect);

    



    void emitIndirectObject(SkWStream* stream, SkPDFCatalog* catalog);

    


    size_t getIndirectOutputSize(SkPDFCatalog* catalog);

    




    static void AddResourceHelper(SkPDFObject* resource,
                                  SkTDArray<SkPDFObject*>* list);

    








    static void GetResourcesHelper(
            const SkTDArray<SkPDFObject*>* resources,
            const SkTSet<SkPDFObject*>& knownResourceObjects,
            SkTSet<SkPDFObject*>* newResourceObjects);

protected:
    





    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect) = 0;

        typedef SkRefCnt INHERITED;
};





class SkPDFObjRef : public SkPDFObject {
public:
    SK_DECLARE_INST_COUNT(SkPDFObjRef)

    


    explicit SkPDFObjRef(SkPDFObject* obj);
    virtual ~SkPDFObjRef();

    
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);
    virtual size_t getOutputSize(SkPDFCatalog* catalog, bool indirect);

private:
    SkAutoTUnref<SkPDFObject> fObj;

    typedef SkPDFObject INHERITED;
};





class SkPDFInt : public SkPDFObject {
public:
    SK_DECLARE_INST_COUNT(SkPDFInt)

    


    explicit SkPDFInt(int32_t value);
    virtual ~SkPDFInt();

    
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);

private:
    int32_t fValue;

    typedef SkPDFObject INHERITED;
};





class SkPDFBool : public SkPDFObject {
public:
    SK_DECLARE_INST_COUNT(SkPDFBool)

    


    explicit SkPDFBool(bool value);
    virtual ~SkPDFBool();

    
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);
    virtual size_t getOutputSize(SkPDFCatalog* catalog, bool indirect);

private:
    bool fValue;

    typedef SkPDFObject INHERITED;
};





class SkPDFScalar : public SkPDFObject {
public:
    SK_DECLARE_INST_COUNT(SkPDFScalar)

    


    explicit SkPDFScalar(SkScalar value);
    virtual ~SkPDFScalar();

    static void Append(SkScalar value, SkWStream* stream);

    
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);

private:
    SkScalar fValue;

    typedef SkPDFObject INHERITED;
};





class SkPDFString : public SkPDFObject {
public:
    SK_DECLARE_INST_COUNT(SkPDFString)

    


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

    typedef SkPDFObject INHERITED;
};





class SkPDFName : public SkPDFObject {
public:
    SK_DECLARE_INST_COUNT(SkPDFName)

    


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

    typedef SkPDFObject INHERITED;
};





class SkPDFArray : public SkPDFObject {
public:
    SK_DECLARE_INST_COUNT(SkPDFArray)

    

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

    typedef SkPDFObject INHERITED;
};





class SkPDFDict : public SkPDFObject {
public:
    SK_DECLARE_INST_COUNT(SkPDFDict)

    

    SkPDFDict();

    


    explicit SkPDFDict(const char type[]);

    virtual ~SkPDFDict();

    
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);
    virtual size_t getOutputSize(SkPDFCatalog* catalog, bool indirect);

    

    int size() const;

    




    SkPDFObject* insert(SkPDFName* key, SkPDFObject* value);

    





    SkPDFObject* insert(const char key[], SkPDFObject* value);

    



    void insertInt(const char key[], int32_t value);

    



    void insertInt(const char key[], size_t value) {
        this->insertInt(key, SkToS32(value));
    }

    



    void insertScalar(const char key[], SkScalar value);

    



    void insertName(const char key[], const char name[]);

    



    void insertName(const char key[], const SkString& name) {
        this->insertName(key, name.c_str());
    }

    

    void clear();

protected:
    

    void remove(const char key[]);

    


    void mergeFrom(const SkPDFDict& other);

private:
    struct Rec {
        SkPDFName* key;
        SkPDFObject* value;
        Rec(SkPDFName* k, SkPDFObject* v) : key(k), value(v) {}
    };

    static const int kMaxLen = 4095;

    mutable SkMutex fMutex;  
    SkTDArray<struct Rec> fValue;

    SkPDFObject* append(SkPDFName* key, SkPDFObject* value);

    typedef SkPDFObject INHERITED;
};

#endif
