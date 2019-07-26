



#include "nsError.h"
#include "mozilla/Assertions.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/NullPtr.h"

namespace mozilla {

struct StackBlock;
struct StackMark;
class AutoStackArena;


class StackArena {
private:
  friend class AutoStackArena;
  StackArena();
  ~StackArena();

  nsresult Init() { return mBlocks ? NS_OK : NS_ERROR_OUT_OF_MEMORY; }

  
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
      gStackArena->Init();
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
    MOZ_ASSERT(aSize <= 4044);
    return gStackArena->Allocate(aSize);
  }

private:
  static StackArena* gStackArena;
  bool mOwnsStackArena;
};

} 
