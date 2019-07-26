








#ifndef SkEventSink_DEFINED
#define SkEventSink_DEFINED

#include "SkRefCnt.h"
#include "SkEvent.h"

struct SkTagList;





class SkEventSink : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkEventSink)

             SkEventSink();
    virtual ~SkEventSink();

    



    SkEventSinkID getSinkID() const { return fID; }

    



    bool doEvent(const SkEvent&);

    


    bool doQuery(SkEvent* query);

    



    void addListenerID(SkEventSinkID sinkID);

    



    void copyListeners(const SkEventSink& from);

    



    void removeListenerID(SkEventSinkID);

    


    bool hasListeners() const;

    



    void postToListeners(const SkEvent& evt, SkMSec delay = 0);

    enum EventResult {
        kHandled_EventResult,       
        kNotHandled_EventResult,    
        kSinkNotFound_EventResult   
    };

    


    static EventResult DoEvent(const SkEvent&);

    


    static SkEventSink* FindSink(SkEventSinkID);

protected:
    


    virtual bool onEvent(const SkEvent&);
    virtual bool onQuery(SkEvent*);

    SkTagList*  findTagList(U8CPU tag) const;
    void        addTagList(SkTagList*);
    void        removeTagList(U8CPU tag);

private:
    SkEventSinkID   fID;
    SkTagList*      fTagHead;

    
    SkEventSink*    fNextSink;

    typedef SkRefCnt INHERITED;
};

#endif

