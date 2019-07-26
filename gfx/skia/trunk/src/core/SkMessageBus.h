






#ifndef SkMessageBus_DEFINED
#define SkMessageBus_DEFINED

#include "SkOnce.h"
#include "SkTDArray.h"
#include "SkThread.h"
#include "SkTypes.h"

template <typename Message>
class SkMessageBus : SkNoncopyable {
public:
    
    static void Post(const Message& m);

    class Inbox {
    public:
        Inbox();
        ~Inbox();

        
        void poll(SkTDArray<Message>* out);

    private:
        SkTDArray<Message> fMessages;
        SkMutex            fMessagesMutex;

        friend class SkMessageBus;
        void receive(const Message& m);  
    };

private:
    SkMessageBus();
    static SkMessageBus* Get();
    static void New(SkMessageBus**);

    SkTDArray<Inbox*> fInboxes;
    SkMutex           fInboxesMutex;
};



#define DECLARE_SKMESSAGEBUS_MESSAGE(Message)             \
    template <>                                           \
    SkMessageBus<Message>* SkMessageBus<Message>::Get() { \
        static SkMessageBus<Message>* bus = NULL;         \
        SK_DECLARE_STATIC_ONCE(once);                     \
        SkOnce(&once, &New, &bus);                        \
        SkASSERT(bus != NULL);                            \
        return bus;                                       \
    }



template<typename Message>
SkMessageBus<Message>::Inbox::Inbox() {
    
    SkMessageBus<Message>* bus = SkMessageBus<Message>::Get();
    SkAutoMutexAcquire lock(bus->fInboxesMutex);
    bus->fInboxes.push(this);
}

template<typename Message>
SkMessageBus<Message>::Inbox::~Inbox() {
    
    SkMessageBus<Message>* bus = SkMessageBus<Message>::Get();
    SkAutoMutexAcquire lock(bus->fInboxesMutex);
    
    for (int i = 0; i < bus->fInboxes.count(); i++) {
        if (this == bus->fInboxes[i]) {
            bus->fInboxes.removeShuffle(i);
            break;
        }
    }
}

template<typename Message>
void SkMessageBus<Message>::Inbox::receive(const Message& m) {
    SkAutoMutexAcquire lock(fMessagesMutex);
    fMessages.push(m);
}

template<typename Message>
void SkMessageBus<Message>::Inbox::poll(SkTDArray<Message>* messages) {
    SkASSERT(NULL != messages);
    messages->reset();
    SkAutoMutexAcquire lock(fMessagesMutex);
    messages->swap(fMessages);
}



template <typename Message>
SkMessageBus<Message>::SkMessageBus() {}

template <typename Message>
 void SkMessageBus<Message>::New(SkMessageBus<Message>** bus) {
    *bus = new SkMessageBus<Message>();
}

template <typename Message>
 void SkMessageBus<Message>::Post(const Message& m) {
    SkMessageBus<Message>* bus = SkMessageBus<Message>::Get();
    SkAutoMutexAcquire lock(bus->fInboxesMutex);
    for (int i = 0; i < bus->fInboxes.count(); i++) {
        bus->fInboxes[i]->receive(m);
    }
}

#endif  
