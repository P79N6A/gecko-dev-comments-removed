




#ifndef mozilla_a11y_EventQueue_h_
#define mozilla_a11y_EventQueue_h_

#include "AccEvent.h"

namespace mozilla {
namespace a11y {

class DocAccessible;




class EventQueue
{
protected:
  explicit EventQueue(DocAccessible* aDocument) : mDocument(aDocument) { }

  


  bool PushEvent(AccEvent* aEvent);

  


  void ProcessEventQueue();

private:
  EventQueue(const EventQueue&) = delete;
  EventQueue& operator = (const EventQueue&) = delete;

  
  


  void CoalesceEvents();

  


  void CoalesceReorderEvents(AccEvent* aTailEvent);

  


  void CoalesceSelChangeEvents(AccSelChangeEvent* aTailEvent,
                               AccSelChangeEvent* aThisEvent,
                               uint32_t aThisIndex);

  


  void SendIPCEvent(AccEvent* aEvent) const;

  


  void CoalesceTextChangeEventsFor(AccHideEvent* aTailEvent,
                                   AccHideEvent* aThisEvent);
  void CoalesceTextChangeEventsFor(AccShowEvent* aTailEvent,
                                   AccShowEvent* aThisEvent);

  




   void CreateTextChangeEventFor(AccMutationEvent* aEvent);

protected:

  


  DocAccessible* mDocument;

  



  nsTArray<nsRefPtr<AccEvent> > mEvents;
};

} 
} 

#endif 
