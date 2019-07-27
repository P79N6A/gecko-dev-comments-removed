





#ifndef mozilla_HoldDropJSObjects_h
#define mozilla_HoldDropJSObjects_h

#include "mozilla/TypeTraits.h"
#include "nsCycleCollectionParticipant.h"

class nsISupports;
class nsScriptObjectTracer;



namespace mozilla {
namespace cyclecollector {


void HoldJSObjectsImpl(void* aHolder, nsScriptObjectTracer* aTracer);
void HoldJSObjectsImpl(nsISupports* aHolder);
void DropJSObjectsImpl(void* aHolder);
void DropJSObjectsImpl(nsISupports* aHolder);

} 


template<class T, bool isISupports = IsBaseOf<nsISupports, T>::value>
struct HoldDropJSObjectsHelper
{
  static void Hold(T* aHolder)
  {
    cyclecollector::HoldJSObjectsImpl(aHolder,
                                      NS_CYCLE_COLLECTION_PARTICIPANT(T));
  }
  static void Drop(T* aHolder)
  {
    cyclecollector::DropJSObjectsImpl(aHolder);
  }
};

template<class T>
struct HoldDropJSObjectsHelper<T, true>
{
  static void Hold(T* aHolder)
  {
    cyclecollector::HoldJSObjectsImpl(ToSupports(aHolder));
  }
  static void Drop(T* aHolder)
  {
    cyclecollector::DropJSObjectsImpl(ToSupports(aHolder));
  }
};


template<class T>
void
HoldJSObjects(T* aHolder)
{
  HoldDropJSObjectsHelper<T>::Hold(aHolder);
}

template<class T>
void
DropJSObjects(T* aHolder)
{
  HoldDropJSObjectsHelper<T>::Drop(aHolder);
}

} 

#endif 
