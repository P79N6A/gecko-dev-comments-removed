





#ifndef nsCycleCollectionNoteRootCallback_h__
#define nsCycleCollectionNoteRootCallback_h__

class nsCycleCollectionParticipant;
class nsISupports;

class nsCycleCollectionNoteRootCallback
{
public:
  NS_IMETHOD_(void) NoteXPCOMRoot(nsISupports *root) = 0;
  NS_IMETHOD_(void) NoteJSRoot(void *root) = 0;
  NS_IMETHOD_(void) NoteNativeRoot(void *root, nsCycleCollectionParticipant *participant) = 0;

  NS_IMETHOD_(void) NoteWeakMapping(void *map, void *key, void *kdelegate, void *val) = 0;

  bool WantAllTraces() const { return mWantAllTraces; }
protected:
  nsCycleCollectionNoteRootCallback() : mWantAllTraces(false) {}

  bool mWantAllTraces;
};

#endif 
