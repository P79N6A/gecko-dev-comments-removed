





#ifndef nsCycleCollectionNoteRootCallback_h__
#define nsCycleCollectionNoteRootCallback_h__

class nsCycleCollectionParticipant;
class nsISupports;

class nsCycleCollectionNoteRootCallback
{
public:
  NS_IMETHOD_(void) NoteXPCOMRoot(nsISupports* aRoot) = 0;
  NS_IMETHOD_(void) NoteJSRoot(JSObject* aRoot) = 0;
  NS_IMETHOD_(void) NoteNativeRoot(void* aRoot,
                                   nsCycleCollectionParticipant* aParticipant) = 0;

  NS_IMETHOD_(void) NoteWeakMapping(JSObject* aMap, JS::GCCellPtr aKey,
                                    JSObject* aKeyDelegate, JS::GCCellPtr aVal) = 0;

  bool WantAllTraces() const { return mWantAllTraces; }
protected:
  nsCycleCollectionNoteRootCallback() : mWantAllTraces(false) {}

  bool mWantAllTraces;
};

#endif 
