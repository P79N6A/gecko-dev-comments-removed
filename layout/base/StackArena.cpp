



#include "StackArena.h"

namespace mozilla {

#define STACK_ARENA_MARK_INCREMENT 50

#define STACK_ARENA_BLOCK_INCREMENT 4044




struct StackBlock {
   
   
   
   char mBlock[STACK_ARENA_BLOCK_INCREMENT];

   
   
   
   StackBlock* mNext;

   StackBlock() : mNext(nullptr) { }
   ~StackBlock() { }
};




struct StackMark {
   
   StackBlock* mBlock;
   
   
   size_t mPos;
};

StackArena* AutoStackArena::gStackArena;

StackArena::StackArena()
{
  mMarkLength = 0;
  mMarks = nullptr;

  
  mBlocks = new StackBlock();
  mCurBlock = mBlocks;

  mStackTop = 0;
  mPos = 0;
}

StackArena::~StackArena()
{
  
  delete[] mMarks;
  while(mBlocks)
  {
    StackBlock* toDelete = mBlocks;
    mBlocks = mBlocks->mNext;
    delete toDelete;
  }
} 

size_t
StackArena::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
  size_t n = 0;
  StackBlock *block = mBlocks;
  while (block) {
    n += aMallocSizeOf(block);
    block = block->mNext;
  }
  n += aMallocSizeOf(mMarks);
  return n;
}

void
StackArena::Push()
{
  
  
  
  if (mStackTop >= mMarkLength)
  {
    uint32_t newLength = mStackTop + STACK_ARENA_MARK_INCREMENT;
    StackMark* newMarks = new StackMark[newLength];
    if (newMarks) {
      if (mMarkLength)
        memcpy(newMarks, mMarks, sizeof(StackMark)*mMarkLength);
      
      
      for (; mMarkLength < mStackTop; ++mMarkLength) {
        NS_NOTREACHED("should only hit this on out-of-memory");
        newMarks[mMarkLength].mBlock = mCurBlock;
        newMarks[mMarkLength].mPos = mPos;
      }
      delete [] mMarks;
      mMarks = newMarks;
      mMarkLength = newLength;
    }
  }

  
  NS_ASSERTION(mStackTop < mMarkLength, "out of memory");
  if (mStackTop < mMarkLength) {
    mMarks[mStackTop].mBlock = mCurBlock;
    mMarks[mStackTop].mPos = mPos;
  }

  mStackTop++;
}

void*
StackArena::Allocate(size_t aSize)
{
  NS_ASSERTION(mStackTop > 0, "Allocate called without Push");

  
  
  aSize = NS_ROUNDUP<size_t>(aSize, 8);

  
  if (mPos + aSize >= STACK_ARENA_BLOCK_INCREMENT)
  {
    NS_ASSERTION(aSize <= STACK_ARENA_BLOCK_INCREMENT,
                 "Requested memory is greater that our block size!!");
    if (mCurBlock->mNext == nullptr)
      mCurBlock->mNext = new StackBlock();

    mCurBlock =  mCurBlock->mNext;
    mPos = 0;
  }

  
  void *result = mCurBlock->mBlock + mPos;
  mPos += aSize;

  return result;
}

void
StackArena::Pop()
{
  
  NS_ASSERTION(mStackTop > 0, "unmatched pop");
  mStackTop--;

  if (mStackTop >= mMarkLength) {
    
    
    NS_NOTREACHED("out of memory");
    if (mStackTop == 0) {
      
      mCurBlock = mBlocks;
      mPos = 0;
    }
    return;
  }

#ifdef DEBUG
  
  
  {
    StackBlock *block = mMarks[mStackTop].mBlock, *block_end = mCurBlock;
    size_t pos = mMarks[mStackTop].mPos;
    for (; block != block_end; block = block->mNext, pos = 0) {
      memset(block->mBlock + pos, 0xdd, sizeof(block->mBlock) - pos);
    }
    memset(block->mBlock + pos, 0xdd, mPos - pos);
  }
#endif

  mCurBlock = mMarks[mStackTop].mBlock;
  mPos      = mMarks[mStackTop].mPos;
}

} 
