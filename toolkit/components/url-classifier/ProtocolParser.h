




#ifndef ProtocolParser_h__
#define ProtocolParser_h__

#include "HashStore.h"
#include "nsICryptoHMAC.h"

namespace mozilla {
namespace safebrowsing {




class ProtocolParser {
public:
  struct ForwardedUpdate {
    nsCString table;
    nsCString url;
  };

  ProtocolParser();
  ~ProtocolParser();

  nsresult Status() const { return mUpdateStatus; }

  nsresult Init(nsICryptoHash* aHasher);

  void SetCurrentTable(const nsACString& aTable);

  nsresult Begin();
  nsresult AppendStream(const nsACString& aData);

  
  
  TableUpdate *GetTableUpdate(const nsACString& aTable);
  void ForgetTableUpdates() { mTableUpdates.Clear(); }
  nsTArray<TableUpdate*> &GetTableUpdates() { return mTableUpdates; }

  
  const nsTArray<ForwardedUpdate> &Forwards() const { return mForwards; }
  int32_t UpdateWait() { return mUpdateWait; }
  bool ResetRequested() { return mResetRequested; }

private:
  nsresult ProcessControl(bool* aDone);
  nsresult ProcessExpirations(const nsCString& aLine);
  nsresult ProcessChunkControl(const nsCString& aLine);
  nsresult ProcessForward(const nsCString& aLine);
  nsresult AddForward(const nsACString& aUrl);
  nsresult ProcessChunk(bool* done);
  
  nsresult ProcessPlaintextChunk(const nsACString& aChunk);
  nsresult ProcessShaChunk(const nsACString& aChunk);
  nsresult ProcessHostAdd(const Prefix& aDomain, uint8_t aNumEntries,
                          const nsACString& aChunk, uint32_t* aStart);
  nsresult ProcessHostSub(const Prefix& aDomain, uint8_t aNumEntries,
                          const nsACString& aChunk, uint32_t* aStart);
  nsresult ProcessHostAddComplete(uint8_t aNumEntries, const nsACString& aChunk,
                                  uint32_t *aStart);
  nsresult ProcessHostSubComplete(uint8_t numEntries, const nsACString& aChunk,
                                  uint32_t* start);
  
  
  
  nsresult ProcessDigestChunk(const nsACString& aChunk);
  nsresult ProcessDigestAdd(const nsACString& aChunk);
  nsresult ProcessDigestSub(const nsACString& aChunk);
  bool NextLine(nsACString& aLine);

  void CleanupUpdates();

  enum ParserState {
    PROTOCOL_STATE_CONTROL,
    PROTOCOL_STATE_CHUNK
  };
  ParserState mState;

  enum ChunkType {
    
    CHUNK_ADD,
    CHUNK_SUB,
    
    
    CHUNK_ADD_DIGEST,
    CHUNK_SUB_DIGEST
  };

  struct ChunkState {
    ChunkType type;
    uint32_t num;
    uint32_t hashSize;
    uint32_t length;
    void Clear() { num = 0; hashSize = 0; length = 0; }
  };
  ChunkState mChunkState;

  nsCOMPtr<nsICryptoHash> mCryptoHash;

  nsresult mUpdateStatus;
  nsCString mPending;

  uint32_t mUpdateWait;
  bool mResetRequested;

  nsTArray<ForwardedUpdate> mForwards;
  
  nsTArray<TableUpdate*> mTableUpdates;
  
  TableUpdate *mTableUpdate;
};

} 
} 

#endif
