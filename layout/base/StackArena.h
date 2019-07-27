



#ifndef StackArena_h
#define StackArena_h

#include "nsError.h"
#include "mozilla/Assertions.h"
#include "mozilla/MemoryReporting.h"

namespace mozilla {

struct StackBlock;
struct StackMark;
class AutoStackArena;


class StackArena {
private:
  friend class AutoStackArena;
  StackArena();
  ~StackArena();

  
  void* Allocate(size_t aSize);
  void Push();
  void Pop();

  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  
  size_t mPos;

  
  
  StackBlock* mBlocks;

  
  StackBlock* mCurBlock;

  
  StackMark* mMarks;

  
  uint32_t mStackTop;

  
  uint32_t mMarkLength;
};












class MOZ_STACK_CLASS AutoStackArena {
public:
  AutoStackArena()
    : mOwnsStackArena(false)
  {
    if (!gStackArena) {
      gStackArena = new StackArena();
      mOwnsStackArena = true;
    }
    gStackArena->Push();
  }

  ~AutoStackArena() {
    gStackArena->Pop();
    if (mOwnsStackArena) {
      delete gStackArena;
      gStackArena = nullptr;
    }
  }

  static void* Allocate(size_t aSize) {
    return gStackArena->Allocate(aSize);
  }

private:
  static StackArena* gStackArena;
  bool mOwnsStackArena;
};

} 

#endif
