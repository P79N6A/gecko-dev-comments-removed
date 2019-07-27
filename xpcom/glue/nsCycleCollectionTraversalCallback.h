





#ifndef nsCycleCollectionTraversalCallback_h__
#define nsCycleCollectionTraversalCallback_h__

#include "nsISupports.h"

class nsCycleCollectionParticipant;

class NS_NO_VTABLE nsCycleCollectionTraversalCallback
{
public:
  
  
  
  
  NS_IMETHOD_(void) DescribeRefCountedNode(nsrefcnt aRefcount,
                                           const char* aObjName) = 0;
  
  NS_IMETHOD_(void) DescribeGCedNode(bool aIsMarked,
                                     const char* aObjName,
                                     uint64_t aCompartmentAddress = 0) = 0;

  NS_IMETHOD_(void) NoteXPCOMChild(nsISupports* aChild) = 0;
  NS_IMETHOD_(void) NoteJSChild(void* aChild) = 0;
  NS_IMETHOD_(void) NoteNativeChild(void* aChild,
                                    nsCycleCollectionParticipant* aHelper) = 0;

  
  
  
  
  NS_IMETHOD_(void) NoteNextEdgeName(const char* aName) = 0;

  enum
  {
    

    
    
    WANT_DEBUG_INFO = (1 << 0),

    
    
    WANT_ALL_TRACES = (1 << 1)
  };
  uint32_t Flags() const { return mFlags; }
  bool WantDebugInfo() const { return (mFlags & WANT_DEBUG_INFO) != 0; }
  bool WantAllTraces() const { return (mFlags & WANT_ALL_TRACES) != 0; }
protected:
  nsCycleCollectionTraversalCallback() : mFlags(0) {}

  uint32_t mFlags;
};

#endif 
