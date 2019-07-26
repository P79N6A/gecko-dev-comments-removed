





#ifndef nsCycleCollectionJSRuntime_h__
#define nsCycleCollectionJSRuntime_h__

class nsCycleCollectionParticipant;
class nsCycleCollectionNoteRootCallback;
class nsScriptObjectTracer;


struct nsCycleCollectionJSRuntime
{
  virtual nsresult BeginCycleCollection(nsCycleCollectionNoteRootCallback &aCb) = 0;

  





  virtual bool NotifyLeaveMainThread() = 0;
  virtual void NotifyEnterCycleCollectionThread() = 0;
  virtual void NotifyLeaveCycleCollectionThread() = 0;
  virtual void NotifyEnterMainThread() = 0;

  


  virtual void FixWeakMappingGrayBits() = 0;

  


  virtual bool UsefulToMergeZones() = 0;

  


  virtual bool NeedCollect() = 0;

  


  virtual void Collect(uint32_t aReason) = 0;

  


  virtual nsCycleCollectionParticipant *GetParticipant() = 0;

  virtual void AddJSHolder(void* aHolder, nsScriptObjectTracer* aTracer) = 0;
  virtual void RemoveJSHolder(void* aHolder) = 0;

#ifdef DEBUG
  virtual bool TestJSHolder(void* aHolder) = 0;

  virtual void SetObjectToUnlink(void* aObject) = 0;
  virtual void AssertNoObjectsToTrace(void* aPossibleJSHolder) = 0;
#endif
};

#endif 
