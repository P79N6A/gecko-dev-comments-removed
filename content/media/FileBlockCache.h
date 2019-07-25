





#ifndef FILE_BLOCK_CACHE_H_
#define FILE_BLOCK_CACHE_H_

#include "mozilla/Monitor.h"
#include "prio.h"
#include "nsTArray.h"
#include "nsMediaCache.h"
#include "nsDeque.h"

namespace mozilla {































class FileBlockCache : public nsRunnable {
public:
  enum {
    BLOCK_SIZE = nsMediaCacheStream::BLOCK_SIZE
  };

  FileBlockCache();

  ~FileBlockCache();

  
  nsresult Open(PRFileDesc* aFD);

  
  void Close();

  
  nsresult WriteBlock(PRUint32 aBlockIndex, const PRUint8* aData);

  
  NS_IMETHOD Run();

  
  
  
  nsresult Read(PRInt64 aOffset,
                PRUint8* aData,
                PRInt32 aLength,
                PRInt32* aBytes);

  
  
  nsresult MoveBlock(PRInt32 aSourceBlockIndex, PRInt32 aDestBlockIndex);

  
  
  
  struct BlockChange {

    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(BlockChange)

    
    
    BlockChange(const PRUint8* aData)
      : mSourceBlockIndex(-1)
    {
      mData = new PRUint8[BLOCK_SIZE];
      memcpy(mData.get(), aData, BLOCK_SIZE);
    }

    
    
    BlockChange(PRInt32 aSourceBlockIndex)
      : mSourceBlockIndex(aSourceBlockIndex) {}

    nsAutoArrayPtr<PRUint8> mData;
    const PRInt32 mSourceBlockIndex;

    bool IsMove() const {
      return mSourceBlockIndex != -1;
    }
    bool IsWrite() const {
      return mSourceBlockIndex == -1 &&
             mData.get() != nsnull;
    }
  };

  class Int32Queue : private nsDeque {
  public:
    PRInt32 PopFront() {
      PRInt32 front = ObjectAt(0);
      nsDeque::PopFront();
      return front;
    }

    void PushBack(PRInt32 aValue) {
      nsDeque::Push(reinterpret_cast<void*>(aValue));
    }

    bool Contains(PRInt32 aValue) {
      for (PRInt32 i = 0; i < GetSize(); ++i) {
        if (ObjectAt(i) == aValue) {
          return true;
        }
      }
      return false;
    }

    bool IsEmpty() {
      return nsDeque::GetSize() == 0;
    }

  private:
    PRInt32 ObjectAt(PRInt32 aIndex) {
      void* v = nsDeque::ObjectAt(aIndex);
      
      
      return *(reinterpret_cast<PRInt32*>(&v));
    }
  };

private:
  
  
  
  mozilla::Monitor mFileMonitor;
  
  nsresult MoveBlockInFile(PRInt32 aSourceBlockIndex,
                           PRInt32 aDestBlockIndex);
  
  nsresult Seek(PRInt64 aOffset);
  
  nsresult ReadFromFile(PRInt32 aOffset,
                        PRUint8* aDest,
                        PRInt32 aBytesToRead,
                        PRInt32& aBytesRead);
  nsresult WriteBlockToFile(PRInt32 aBlockIndex, const PRUint8* aBlockData);
  
  
  PRFileDesc* mFD;
  
  PRInt64 mFDCurrentPos;

  
  
  
  
  mozilla::Monitor mDataMonitor;
  
  
  
  void EnsureWriteScheduled();
  
  
  
  
  
  nsTArray< nsRefPtr<BlockChange> > mBlockChanges;
  
  
  
  nsCOMPtr<nsIThread> mThread;
  
  
  Int32Queue mChangeIndexList;
  
  
  bool mIsWriteScheduled;
  
  bool mIsOpen;
};

} 

#endif 
