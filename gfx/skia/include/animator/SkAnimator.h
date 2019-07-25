








#ifndef SkAnimator_DEFINED
#define SkAnimator_DEFINED

#include "SkScalar.h"
#include "SkKey.h"
#include "SkEventSink.h"

class SkAnimateMaker;
class SkCanvas;
class SkDisplayable;
class SkEvent;
class SkExtras;
struct SkMemberInfo;
class SkPaint;
struct SkRect;
class SkStream;
class SkTypedArray;
class SkXMLParserError;
class SkDOM;
struct SkDOMNode;



enum SkElementType {
    kElementDummyType
};


enum SkFieldType {
    kFieldDummyType
};









































class SkAnimator : public SkEventSink {
public:
    SkAnimator();
    virtual ~SkAnimator();

    


    void addExtras(SkExtras* extras);

    





    bool appendStream(SkStream* stream);

    






    bool decodeMemory(const void* buffer, size_t size);

    





    virtual bool decodeStream(SkStream* stream);

    




    virtual bool decodeDOM(const SkDOM&, const SkDOMNode*);

    





    bool decodeURI(const char uri[]);

    





    bool doCharEvent(SkUnichar ch);

    









    bool doClickEvent(int state, SkScalar x, SkScalar y);

    





    bool doKeyEvent(SkKey code);
    bool doKeyUpEvent(SkKey code);
    
    



    bool doUserEvent(const SkEvent& evt);

    

    enum DifferenceType {
        kNotDifferent,
        kDifferent,
        kPartiallyDifferent
    };
    










    DifferenceType draw(SkCanvas* canvas, SkPaint* paint, SkMSec time);

    










    DifferenceType draw(SkCanvas* canvas, SkMSec time);

    





    bool findClickEvent(SkScalar x, SkScalar y); 


    




    const SkAnimator* getAnimator(const SkDisplayable* element) const;

    





    int32_t getArrayInt(const SkDisplayable* element, const SkMemberInfo* field, int index);

    





    int32_t getArrayInt(const char* elementID, const char* fieldName, int index);

    





    SkScalar getArrayScalar(const SkDisplayable* element, const SkMemberInfo* field, int index);

    





    SkScalar getArrayScalar(const char* elementID, const char* fieldName, int index);

    





    const char* getArrayString(const SkDisplayable* element, const SkMemberInfo* field, int index);

    





    const char* getArrayString(const char* elementID, const char* fieldName, int index);

    



    const SkDisplayable* getElement(const char* elementID);

    




    SkElementType getElementType(const SkDisplayable* element);

    



    SkElementType getElementType(const char* elementID);

    




    const SkMemberInfo* getField(const SkDisplayable* element, const char* fieldName);

    




    const SkMemberInfo* getField(const char* elementID, const char* fieldName);

    




    SkFieldType getFieldType(const SkMemberInfo* field);

    




    SkFieldType getFieldType(const char* elementID, const char* fieldName);

    


    SkMSec getInterval();

    

    void getInvalBounds(SkRect* inval); 

    

    const SkXMLParserError* getParserError();
    
    

    const char* getParserErrorString();
    
    




    int32_t getInt(const SkDisplayable* element, const SkMemberInfo* field);

    




    int32_t getInt(const char* elementID, const char* fieldName);

    




    SkScalar getScalar(const SkDisplayable* element, const SkMemberInfo* field);

    




    SkScalar getScalar(const char* elementID, const char* fieldName);

    




    const char* getString(const SkDisplayable* element, const SkMemberInfo* field);

    




    const char* getString(const char* elementID, const char* fieldName);

    
    const char* getURIBase();

    
    void initialize();

    

    void reset();
    
    






    bool setArrayInt(const char* elementID, const char* fieldName, const int* array, int count);
    
    






    bool setArrayString(const char* elementID, const char* fieldName, const char** array, int count);
    
    





    bool setInt(const char* elementID, const char* fieldName, int32_t data);

    





    bool setScalar(const char* elementID, const char* fieldName, SkScalar data);

    





    bool setString(const char* elementID, const char* fieldName, const char* data);

    


    void setURIBase(const char* path);

    typedef void* Handler;
    
    void setHostHandler(Handler handler) {
        this->onSetHostHandler(handler);
    }

    



    class Timeline {
    public:
        virtual ~Timeline() {}

        
        virtual SkMSec getMSecs() const = 0;
    };

    



    void setTimeline(const Timeline& );

    static void Init(bool runUnitTests);
    static void Term();
    
    




    void setHostEventSinkID(SkEventSinkID hostID);
    SkEventSinkID getHostEventSinkID() const;
    
    
    void setHostEventSink(SkEventSink* sink) {
        this->setHostEventSinkID(sink ? sink->getSinkID() : 0);
    }
    
    virtual void setJavaOwner(Handler owner);
    
#ifdef SK_DEBUG
    virtual void eventDone(const SkEvent& evt);
    virtual bool isTrackingEvents();
    static bool NoLeaks();
#endif  
    
protected:
    virtual void onSetHostHandler(Handler handler);
    virtual void onEventPost(SkEvent*, SkEventSinkID);
    virtual void onEventPostTime(SkEvent*, SkEventSinkID, SkMSec time);

private:

    bool setArray(SkDisplayable* element, const SkMemberInfo* field, SkTypedArray array);
    bool setArray(const char* elementID, const char* fieldName, SkTypedArray array);
    bool setInt(SkDisplayable* element, const SkMemberInfo* field, int32_t data);
    bool setScalar(SkDisplayable* element, const SkMemberInfo* field, SkScalar data);
    bool setString(SkDisplayable* element, const SkMemberInfo* field, const char* data);
    
    virtual bool onEvent(const SkEvent&);
    SkAnimateMaker* fMaker;
    friend class SkAnimateMaker;
    friend class SkAnimatorScript;
    friend class SkAnimatorScript2;
    friend class SkApply;
    friend class SkDisplayMovie;
    friend class SkDisplayType;
    friend class SkPost;
    friend class SkXMLAnimatorWriter;
};

#endif

