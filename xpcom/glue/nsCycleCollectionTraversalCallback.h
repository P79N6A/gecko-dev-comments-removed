




#ifndef nsCycleCollectionTraversalCallback_h__
#define nsCycleCollectionTraversalCallback_h__

#include "nsISupports.h"

class nsCycleCollectionParticipant;

class NS_NO_VTABLE nsCycleCollectionTraversalCallback
{
public:
  
  
  
  
  NS_IMETHOD_(void) DescribeRefCountedNode(nsrefcnt refcount,
                                           const char* objname) = 0;
  NS_IMETHOD_(void) DescribeGCedNode(bool ismarked,
                                     const char* objname) = 0;

  NS_IMETHOD_(void) NoteXPCOMRoot(nsISupports *root) = 0;
  NS_IMETHOD_(void) NoteJSRoot(void *root) = 0;
  NS_IMETHOD_(void) NoteNativeRoot(void *root, nsCycleCollectionParticipant *participant) = 0;

  NS_IMETHOD_(void) NoteXPCOMChild(nsISupports *child) = 0;
  NS_IMETHOD_(void) NoteJSChild(void *child) = 0;
  NS_IMETHOD_(void) NoteNativeChild(void *child,
                                    nsCycleCollectionParticipant *helper) = 0;

  
  
  
  
  NS_IMETHOD_(void) NoteNextEdgeName(const char* name) = 0;

  NS_IMETHOD_(void) NoteWeakMapping(void *map, void *key, void *kdelegate, void *val) = 0;

  enum {
    

    
    
    WANT_DEBUG_INFO = (1<<0),

    
    
    WANT_ALL_TRACES = (1<<1)
  };
  uint32_t Flags() const { return mFlags; }
  bool WantDebugInfo() const { return (mFlags & WANT_DEBUG_INFO) != 0; }
  bool WantAllTraces() const { return (mFlags & WANT_ALL_TRACES) != 0; }
protected:
  nsCycleCollectionTraversalCallback() : mFlags(0) {}

  uint32_t mFlags;
};

#endif 
