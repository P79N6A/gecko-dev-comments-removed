





#ifndef nsCycleCollectionNoteRootCallback_h__
#define nsCycleCollectionNoteRootCallback_h__

class nsCycleCollectionParticipant;
class nsISupports;

class nsCycleCollectionNoteRootCallback
{
public:
  NS_IMETHOD_(void) NoteXPCOMRoot(nsISupports* aRoot) = 0;
  NS_IMETHOD_(void) NoteJSRoot(void* aRoot) = 0;
  NS_IMETHOD_(void) NoteNativeRoot(void* aRoot,
                                   nsCycleCollectionParticipant* aParticipant) = 0;

  NS_IMETHOD_(void) NoteWeakMapping(void* aMap, void* aKey, void* aKeyDelegate,
                                    void* aVal) = 0;

  bool WantAllTraces() const { return mWantAllTraces; }
protected:
  nsCycleCollectionNoteRootCallback() : mWantAllTraces(false) {}

  bool mWantAllTraces;
};

#endif 
