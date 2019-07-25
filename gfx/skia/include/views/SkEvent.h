








#ifndef SkEvent_DEFINED
#define SkEvent_DEFINED

#include "SkDOM.h"
#include "SkMetaData.h"
#include "SkString.h"






typedef uint32_t SkEventSinkID;








class SkEvent {
public:
    


    typedef bool (*Proc)(const SkEvent& evt);

    SkEvent();
    explicit SkEvent(const SkString& type, SkEventSinkID = 0);
    explicit SkEvent(const char type[], SkEventSinkID = 0);
    SkEvent(const SkEvent& src);
    ~SkEvent();

    
    void getType(SkString* str) const;

    
    bool isType(const SkString& str) const;

    
    bool isType(const char type[], size_t len = 0) const;

    


    void setType(const SkString&);

    


    void setType(const char type[], size_t len = 0);

    






    SkEventSinkID getTargetID() const { return fTargetID; }

    







    SkEvent* setTargetID(SkEventSinkID targetID) {
        fTargetProc = NULL;
        fTargetID = targetID;
        return this;
    }

    






    Proc getTargetProc() const { return fTargetProc; }

    







    SkEvent* setTargetProc(Proc proc) {
        fTargetID = 0;
        fTargetProc = proc;
        return this;
    }
    
    


    uint32_t getFast32() const { return f32; }

    


    void setFast32(uint32_t x) { f32 = x; }

    



    bool findS32(const char name[], int32_t* value = NULL) const { return fMeta.findS32(name, value); }
    



    bool findScalar(const char name[], SkScalar* value = NULL) const { return fMeta.findScalar(name, value); }
    



    const SkScalar* findScalars(const char name[], int* count, SkScalar values[] = NULL) const { return fMeta.findScalars(name, count, values); }
    

    const char* findString(const char name[]) const { return fMeta.findString(name); }
    



    bool findPtr(const char name[], void** value) const { return fMeta.findPtr(name, value); }
    bool findBool(const char name[], bool* value) const { return fMeta.findBool(name, value); }
    const void* findData(const char name[], size_t* byteCount = NULL) const {
        return fMeta.findData(name, byteCount);
    }

    
    bool hasS32(const char name[], int32_t value) const { return fMeta.hasS32(name, value); }
    
    bool hasScalar(const char name[], SkScalar value) const { return fMeta.hasScalar(name, value); }
    
    bool hasString(const char name[], const char value[]) const { return fMeta.hasString(name, value); }
    
    bool hasPtr(const char name[], void* value) const { return fMeta.hasPtr(name, value); }
    bool hasBool(const char name[], bool value) const { return fMeta.hasBool(name, value); }
    bool hasData(const char name[], const void* data, size_t byteCount) const {
        return fMeta.hasData(name, data, byteCount);
    }

    
    void setS32(const char name[], int32_t value) { fMeta.setS32(name, value); }
    
    void setScalar(const char name[], SkScalar value) { fMeta.setScalar(name, value); }
    
    SkScalar* setScalars(const char name[], int count, const SkScalar values[] = NULL) { return fMeta.setScalars(name, count, values); }
    
    void setString(const char name[], const SkString& value) { fMeta.setString(name, value.c_str()); }
    
    void setString(const char name[], const char value[]) { fMeta.setString(name, value); }
    
    void setPtr(const char name[], void* value) { fMeta.setPtr(name, value); }
    void setBool(const char name[], bool value) { fMeta.setBool(name, value); }
    void setData(const char name[], const void* data, size_t byteCount) {
        fMeta.setData(name, data, byteCount);
    }

    
    SkMetaData& getMetaData() { return fMeta; }
    
    const SkMetaData& getMetaData() const { return fMeta; }

    
    void inflate(const SkDOM&, const SkDOM::Node*);

    SkDEBUGCODE(void dump(const char title[] = NULL);)

    

    





    void post() {
        return this->postDelay(0);
    }
    
    






    void postDelay(SkMSec delay);
    
    







    void postTime(SkMSec time);

    
    
    

    



    static void Init();
    


    static void Term();

    


    static bool ProcessEvent();
    



    static void ServiceQueueTimer();

    



    static int CountEventsOnQueue();

    
    
    

    


    static void SignalNonEmptyQueue();
    


    static void SignalQueueTimer(SkMSec delay);

#ifndef SK_USE_WXWIDGETS
#ifdef SK_BUILD_FOR_WIN
    static bool WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
#elif defined(SK_BUILD_FOR_UNIXx)
  static uint32_t HandleTimer(uint32_t, void*);
  static bool WndProc(Display*, Window, XEvent&);
#endif
#else
    
    
#endif

private:
    SkMetaData      fMeta;
    mutable char*   fType;  
    uint32_t        f32;

    
    SkEventSinkID   fTargetID;
    Proc            fTargetProc;

    
    SkMSec          fTime;
    SkEvent*        fNextEvent; 

    void initialize(const char* type, size_t typeLen, SkEventSinkID);

    static bool Enqueue(SkEvent* evt);
    static SkMSec EnqueueTime(SkEvent* evt, SkMSec time);
    static SkEvent* Dequeue();
    static bool     QHasEvents();
};

#endif

