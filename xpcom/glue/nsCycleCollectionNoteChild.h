








#ifndef nsCycleCollectionNoteChild_h__
#define nsCycleCollectionNoteChild_h__

#include "nsCycleCollectionTraversalCallback.h"
#include "mozilla/Likely.h"
#include "mozilla/TypeTraits.h"

enum
{
  CycleCollectionEdgeNameArrayFlag = 1
};


void
CycleCollectionNoteEdgeNameImpl(nsCycleCollectionTraversalCallback& aCallback,
                                const char* aName,
                                uint32_t aFlags = 0);


MOZ_ALWAYS_INLINE void
CycleCollectionNoteEdgeName(nsCycleCollectionTraversalCallback& aCallback,
                            const char* aName,
                            uint32_t aFlags = 0)
{
  if (MOZ_UNLIKELY(aCallback.WantDebugInfo())) {
    CycleCollectionNoteEdgeNameImpl(aCallback, aName, aFlags);
  }
}

#define NS_CYCLE_COLLECTION_INNERCLASS                                         \
        cycleCollection

#define NS_CYCLE_COLLECTION_INNERNAME                                          \
        _cycleCollectorGlobal

#define NS_CYCLE_COLLECTION_PARTICIPANT(_class)                                \
        _class::NS_CYCLE_COLLECTION_INNERCLASS::GetParticipant()

template<typename T>
nsISupports*
ToSupports(T* aPtr, typename T::NS_CYCLE_COLLECTION_INNERCLASS* aDummy = 0)
{
  return T::NS_CYCLE_COLLECTION_INNERCLASS::Upcast(aPtr);
}



template<typename T,
         bool IsXPCOM = mozilla::IsBaseOf<nsISupports, T>::value>
struct CycleCollectionNoteChildImpl
{
};

template<typename T>
struct CycleCollectionNoteChildImpl<T, true>
{
  static void Run(nsCycleCollectionTraversalCallback& aCallback, T* aChild)
  {
    aCallback.NoteXPCOMChild(ToSupports(aChild));
  }
};

template<typename T>
struct CycleCollectionNoteChildImpl<T, false>
{
  static void Run(nsCycleCollectionTraversalCallback& aCallback, T* aChild)
  {
    aCallback.NoteNativeChild(aChild, NS_CYCLE_COLLECTION_PARTICIPANT(T));
  }
};




template<typename T>
inline void
CycleCollectionNoteChild(nsCycleCollectionTraversalCallback& aCallback,
                         T* aChild, const char* aName, uint32_t aFlags)
{
  CycleCollectionNoteEdgeName(aCallback, aName, aFlags);
  CycleCollectionNoteChildImpl<T>::Run(aCallback, aChild);
}

template<typename T>
inline void
CycleCollectionNoteChild(nsCycleCollectionTraversalCallback& aCallback,
                         T* aChild, const char* aName)
{
  CycleCollectionNoteChild(aCallback, aChild, aName, 0);
}

#endif 
