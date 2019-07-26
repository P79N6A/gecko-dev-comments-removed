





#ifndef nsCycleCollectionJSRuntime_h__
#define nsCycleCollectionJSRuntime_h__

class nsCycleCollectionParticipant;
class nsCycleCollectionTraversalCallback;


struct nsCycleCollectionJSRuntime
{
  virtual nsresult BeginCycleCollection(nsCycleCollectionTraversalCallback &aCb) = 0;

  





  virtual bool NotifyLeaveMainThread() = 0;
  virtual void NotifyEnterCycleCollectionThread() = 0;
  virtual void NotifyLeaveCycleCollectionThread() = 0;
  virtual void NotifyEnterMainThread() = 0;

  


  virtual void FixWeakMappingGrayBits() = 0;

  


  virtual bool NeedCollect() = 0;

  


  virtual void Collect(uint32_t aReason) = 0;

  


  virtual nsCycleCollectionParticipant *GetParticipant() = 0;

#ifdef DEBUG
  virtual void SetObjectToUnlink(void* aObject) = 0;
  virtual void AssertNoObjectsToTrace(void* aPossibleJSHolder) = 0;
#endif
};

#endif 
