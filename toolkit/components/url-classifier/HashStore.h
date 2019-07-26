



#ifndef HashStore_h__
#define HashStore_h__

#include "Entries.h"
#include "ChunkSet.h"

#include "nsString.h"
#include "nsTArray.h"
#include "nsIFile.h"
#include "nsIFileStreams.h"
#include "nsCOMPtr.h"

namespace mozilla {
namespace safebrowsing {

class TableUpdate {
public:
  TableUpdate(const nsACString& aTable)
      : mTable(aTable), mLocalUpdate(false) {}
  const nsCString& TableName() const { return mTable; }

  bool Empty() const {
    return mAddChunks.Length() == 0 &&
      mSubChunks.Length() == 0 &&
      mAddExpirations.Length() == 0 &&
      mSubExpirations.Length() == 0 &&
      mAddPrefixes.Length() == 0 &&
      mSubPrefixes.Length() == 0 &&
      mAddCompletes.Length() == 0 &&
      mSubCompletes.Length() == 0;
  }

  void NewAddChunk(uint32_t aChunk) { mAddChunks.Set(aChunk); }
  void NewSubChunk(uint32_t aChunk) { mSubChunks.Set(aChunk); }

  void NewAddExpiration(uint32_t aChunk) { mAddExpirations.Set(aChunk); }
  void NewSubExpiration(uint32_t aChunk) { mSubExpirations.Set(aChunk); }

  void NewAddPrefix(uint32_t aAddChunk, const Prefix& aPrefix);
  void NewSubPrefix(uint32_t aAddChunk, const Prefix& aPrefix, uint32_t aSubChunk);
  void NewAddComplete(uint32_t aChunk, const Completion& aCompletion);
  void NewSubComplete(uint32_t aAddChunk, const Completion& aCompletion,
                      uint32_t aSubChunk);
  void SetLocalUpdate(void) { mLocalUpdate = true; }
  bool IsLocalUpdate(void) { return mLocalUpdate; }

  ChunkSet& AddChunks() { return mAddChunks; }
  ChunkSet& SubChunks() { return mSubChunks; }

  ChunkSet& AddExpirations() { return mAddExpirations; }
  ChunkSet& SubExpirations() { return mSubExpirations; }

  AddPrefixArray& AddPrefixes() { return mAddPrefixes; }
  SubPrefixArray& SubPrefixes() { return mSubPrefixes; }
  AddCompleteArray& AddCompletes() { return mAddCompletes; }
  SubCompleteArray& SubCompletes() { return mSubCompletes; }

private:
  nsCString mTable;
  
  bool mLocalUpdate;

  ChunkSet mAddChunks;
  ChunkSet mSubChunks;
  ChunkSet mAddExpirations;
  ChunkSet mSubExpirations;
  AddPrefixArray mAddPrefixes;
  SubPrefixArray mSubPrefixes;
  AddCompleteArray mAddCompletes;
  SubCompleteArray mSubCompletes;
};

class HashStore {
public:
  HashStore(const nsACString& aTableName, nsIFile* aStoreFile);
  ~HashStore();

  const nsCString& TableName() const { return mTableName; }

  nsresult Open();
  nsresult AugmentAdds(const nsTArray<uint32_t>& aPrefixes);

  ChunkSet& AddChunks() { return mAddChunks; }
  ChunkSet& SubChunks() { return mSubChunks; }
  AddPrefixArray& AddPrefixes() { return mAddPrefixes; }
  AddCompleteArray& AddCompletes() { return mAddCompletes; }
  SubPrefixArray& SubPrefixes() { return mSubPrefixes; }
  SubCompleteArray& SubCompletes() { return mSubCompletes; }

  
  
  
  
  nsresult BeginUpdate();

  
  nsresult ApplyUpdate(TableUpdate &aUpdate);

  
  nsresult Expire();

  
  nsresult Rebuild();

  
  
  
  nsresult WriteFile();

  
  void ClearCompletes();

private:
  nsresult Reset();

  nsresult ReadHeader();
  nsresult SanityCheck();
  nsresult CalculateChecksum(nsAutoCString& aChecksum, int64_t aSize, bool aChecksumPresent);
  nsresult CheckChecksum(nsIFile* aStoreFile);
  void UpdateHeader();

  nsresult ReadChunkNumbers();
  nsresult ReadHashes();
  nsresult ReadAddPrefixes();
  nsresult ReadSubPrefixes();

  nsresult WriteAddPrefixes(nsIOutputStream* aOut);
  nsresult WriteSubPrefixes(nsIOutputStream* aOut);

  nsresult ProcessSubs();

  struct Header {
    uint32_t magic;
    uint32_t version;
    uint32_t numAddChunks;
    uint32_t numSubChunks;
    uint32_t numAddPrefixes;
    uint32_t numSubPrefixes;
    uint32_t numAddCompletes;
    uint32_t numSubCompletes;
  };

  Header mHeader;

  nsCString mTableName;
  nsCOMPtr<nsIFile> mStoreDirectory;

  bool mInUpdate;

  nsCOMPtr<nsIInputStream> mInputStream;

  ChunkSet mAddChunks;
  ChunkSet mSubChunks;

  ChunkSet mAddExpirations;
  ChunkSet mSubExpirations;

  AddPrefixArray mAddPrefixes;
  AddCompleteArray mAddCompletes;
  SubPrefixArray mSubPrefixes;
  SubCompleteArray mSubCompletes;
};

}
}

#endif
