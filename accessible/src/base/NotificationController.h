




#ifndef mozilla_a11y_NotificationController_h_
#define mozilla_a11y_NotificationController_h_

#include "EventQueue.h"

#include "nsCycleCollectionParticipant.h"
#include "nsRefreshDriver.h"

#ifdef A11Y_LOG
#include "Logging.h"
#endif

namespace mozilla {
namespace a11y {

class DocAccessible;




class Notification
{
public:
  virtual ~Notification() { }

  NS_INLINE_DECL_REFCOUNTING(Notification)

  


  virtual void Process() = 0;

protected:
  Notification() { }

private:
  Notification(const Notification&);
  Notification& operator = (const Notification&);
};









template<class Class, class Arg>
class TNotification : public Notification
{
public:
  typedef void (Class::*Callback)(Arg*);

  TNotification(Class* aInstance, Callback aCallback, Arg* aArg) :
    mInstance(aInstance), mCallback(aCallback), mArg(aArg) { }
  virtual ~TNotification() { mInstance = nullptr; }

  virtual void Process()
  {
    (mInstance->*mCallback)(mArg);

    mInstance = nullptr;
    mCallback = nullptr;
    mArg = nullptr;
  }

private:
  TNotification(const TNotification&);
  TNotification& operator = (const TNotification&);

  Class* mInstance;
  Callback mCallback;
  nsRefPtr<Arg> mArg;
};




class NotificationController : public EventQueue,
                               public nsARefreshObserver
{
public:
  NotificationController(DocAccessible* aDocument, nsIPresShell* aPresShell);
  virtual ~NotificationController();

  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(NotificationController)

  


  void Shutdown();

  


  void QueueEvent(AccEvent* aEvent)
  {
    if (PushEvent(aEvent))
      ScheduleProcessing();
  }

  


  void ScheduleChildDocBinding(DocAccessible* aDocument);

  


  inline void ScheduleTextUpdate(nsIContent* aTextNode)
  {
    if (mTextHash.PutEntry(aTextNode))
      ScheduleProcessing();
  }

  


  void ScheduleContentInsertion(Accessible* aContainer,
                                nsIContent* aStartChildNode,
                                nsIContent* aEndChildNode);

  







  template<class Class, class Arg>
  inline void HandleNotification(Class* aInstance,
                                 typename TNotification<Class, Arg>::Callback aMethod,
                                 Arg* aArg)
  {
    if (!IsUpdatePending()) {
#ifdef A11Y_LOG
      if (mozilla::a11y::logging::IsEnabled(mozilla::a11y::logging::eNotifications))
        mozilla::a11y::logging::Text("sync notification processing");
#endif
      (aInstance->*aMethod)(aArg);
      return;
    }

    nsRefPtr<Notification> notification =
      new TNotification<Class, Arg>(aInstance, aMethod, aArg);
    if (notification && mNotifications.AppendElement(notification))
      ScheduleProcessing();
  }

  





  template<class Class, class Arg>
  inline void ScheduleNotification(Class* aInstance,
                                   typename TNotification<Class, Arg>::Callback aMethod,
                                   Arg* aArg)
  {
    nsRefPtr<Notification> notification =
      new TNotification<Class, Arg>(aInstance, aMethod, aArg);
    if (notification && mNotifications.AppendElement(notification))
      ScheduleProcessing();
  }

#ifdef DEBUG
  bool IsUpdating() const
    { return mObservingState == eRefreshProcessingForUpdate; }
#endif

protected:
  nsCycleCollectingAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD

  



  void ScheduleProcessing();

  


  bool IsUpdatePending();

private:
  NotificationController(const NotificationController&);
  NotificationController& operator = (const NotificationController&);

  
  virtual void WillRefresh(mozilla::TimeStamp aTime);

private:
  



  enum eObservingState {
    eNotObservingRefresh,
    eRefreshObserving,
    eRefreshProcessing,
    eRefreshProcessingForUpdate
  };
  eObservingState mObservingState;

  


  nsIPresShell* mPresShell;

  


  nsTArray<nsRefPtr<DocAccessible> > mHangingChildDocuments;

  


  class ContentInsertion
  {
  public:
    ContentInsertion(DocAccessible* aDocument, Accessible* aContainer);
    virtual ~ContentInsertion() { mDocument = nullptr; }

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(ContentInsertion)
    NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(ContentInsertion)

    bool InitChildList(nsIContent* aStartChildNode, nsIContent* aEndChildNode);
    void Process();

  private:
    ContentInsertion();
    ContentInsertion(const ContentInsertion&);
    ContentInsertion& operator = (const ContentInsertion&);

    
    
    
    DocAccessible* mDocument;

    
    nsRefPtr<Accessible> mContainer;

    
    nsTArray<nsCOMPtr<nsIContent> > mInsertedContent;
  };

  



  nsTArray<nsRefPtr<ContentInsertion> > mContentInsertions;

  template<class T>
  class nsCOMPtrHashKey : public PLDHashEntryHdr
  {
  public:
    typedef T* KeyType;
    typedef const T* KeyTypePointer;

    nsCOMPtrHashKey(const T* aKey) : mKey(const_cast<T*>(aKey)) {}
    nsCOMPtrHashKey(const nsPtrHashKey<T> &aToCopy) : mKey(aToCopy.mKey) {}
    ~nsCOMPtrHashKey() { }

    KeyType GetKey() const { return mKey; }
    bool KeyEquals(KeyTypePointer aKey) const { return aKey == mKey; }

    static KeyTypePointer KeyToPointer(KeyType aKey) { return aKey; }
    static PLDHashNumber HashKey(KeyTypePointer aKey)
      { return NS_PTR_TO_INT32(aKey) >> 2; }

    enum { ALLOW_MEMMOVE = true };

   protected:
     nsCOMPtr<T> mKey;
  };

  


  nsTHashtable<nsCOMPtrHashKey<nsIContent> > mTextHash;

  


  static PLDHashOperator TextEnumerator(nsCOMPtrHashKey<nsIContent>* aEntry,
                                        void* aUserArg);

  



  nsTArray<nsRefPtr<Notification> > mNotifications;
};

} 
} 

#endif
