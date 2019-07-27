





#ifndef FILE_BLOCK_CACHE_H_
#define FILE_BLOCK_CACHE_H_

#include "mozilla/Attributes.h"
#include "mozilla/Monitor.h"
#include "nsTArray.h"
#include "MediaCache.h"
#include "nsDeque.h"
#include "nsThreadUtils.h"

struct PRFileDesc;

namespace mozilla {































class FileBlockCache : public nsRunnable {
public:
  enum {
    BLOCK_SIZE = MediaCacheStream::BLOCK_SIZE
  };

  FileBlockCache();

protected:
  ~FileBlockCache();

public:
  
  nsresult Open(PRFileDesc* aFD);

  
  void Close();

  
  nsresult WriteBlock(uint32_t aBlockIndex, const uint8_t* aData);

  
  NS_IMETHOD Run() MOZ_OVERRIDE;

  
  
  
  nsresult Read(int64_t aOffset,
                uint8_t* aData,
                int32_t aLength,
                int32_t* aBytes);

  
  
  nsresult MoveBlock(int32_t aSourceBlockIndex, int32_t aDestBlockIndex);

  
  
  
  struct BlockChange MOZ_FINAL {

    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(BlockChange)

    
    
    explicit BlockChange(const uint8_t* aData)
      : mSourceBlockIndex(-1)
    {
      mData = new uint8_t[BLOCK_SIZE];
      memcpy(mData.get(), aData, BLOCK_SIZE);
    }

    
    
    explicit BlockChange(int32_t aSourceBlockIndex)
      : mSourceBlockIndex(aSourceBlockIndex) {}

    nsAutoArrayPtr<uint8_t> mData;
    const int32_t mSourceBlockIndex;

    bool IsMove() const {
      return mSourceBlockIndex != -1;
    }
    bool IsWrite() const {
      return mSourceBlockIndex == -1 &&
             mData.get() != nullptr;
    }

  private:
    
    ~BlockChange()
    {
    }
  };

  class Int32Queue : private nsDeque {
  public:
    int32_t PopFront() {
      int32_t front = ObjectAt(0);
      nsDeque::PopFront();
      return front;
    }

    void PushBack(int32_t aValue) {
      nsDeque::Push(reinterpret_cast<void*>(aValue));
    }

    bool Contains(int32_t aValue) {
      for (int32_t i = 0; i < GetSize(); ++i) {
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
    int32_t ObjectAt(int32_t aIndex) {
      void* v = nsDeque::ObjectAt(aIndex);
      return reinterpret_cast<uintptr_t>(v);
    }
  };

private:
  int64_t BlockIndexToOffset(int32_t aBlockIndex) {
    return static_cast<int64_t>(aBlockIndex) * BLOCK_SIZE;
  }

  
  
  
  Monitor mFileMonitor;
  
  nsresult MoveBlockInFile(int32_t aSourceBlockIndex,
                           int32_t aDestBlockIndex);
  
  nsresult Seek(int64_t aOffset);
  
  nsresult ReadFromFile(int64_t aOffset,
                        uint8_t* aDest,
                        int32_t aBytesToRead,
                        int32_t& aBytesRead);
  nsresult WriteBlockToFile(int32_t aBlockIndex, const uint8_t* aBlockData);
  
  
  PRFileDesc* mFD;
  
  int64_t mFDCurrentPos;

  
  
  
  
  Monitor mDataMonitor;
  
  
  
  void EnsureWriteScheduled();
  
  
  
  
  
  nsTArray< nsRefPtr<BlockChange> > mBlockChanges;
  
  
  
  nsCOMPtr<nsIThread> mThread;
  
  
  Int32Queue mChangeIndexList;
  
  
  bool mIsWriteScheduled;
  
  bool mIsOpen;
};

} 

#endif 
