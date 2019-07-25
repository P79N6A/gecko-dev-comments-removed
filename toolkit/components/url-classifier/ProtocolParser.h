




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
    nsCString mac;
  };

  ProtocolParser(uint32_t aHashKey);
  ~ProtocolParser();

  nsresult Status() const { return mUpdateStatus; }

  nsresult Init(nsICryptoHash* aHasher, bool mPerClientRandomize);

  nsresult InitHMAC(const nsACString& aClientKey,
                    const nsACString& aServerMAC);
  nsresult FinishHMAC();

  void SetCurrentTable(const nsACString& aTable);

  nsresult Begin();
  nsresult AppendStream(const nsACString& aData);

  
  
  TableUpdate *GetTableUpdate(const nsACString& aTable);
  void ForgetTableUpdates() { mTableUpdates.Clear(); }
  nsTArray<TableUpdate*> &GetTableUpdates() { return mTableUpdates; }

  
  const nsTArray<ForwardedUpdate> &Forwards() const { return mForwards; }
  int32 UpdateWait() { return mUpdateWait; }
  bool ResetRequested() { return mResetRequested; }
  bool RekeyRequested() { return mRekeyRequested; }

private:
  nsresult ProcessControl(bool* aDone);
  nsresult ProcessMAC(const nsCString& aLine);
  nsresult ProcessExpirations(const nsCString& aLine);
  nsresult ProcessChunkControl(const nsCString& aLine);
  nsresult ProcessForward(const nsCString& aLine);
  nsresult AddForward(const nsACString& aUrl, const nsACString& aMac);
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
  bool NextLine(nsACString& aLine);

  void CleanupUpdates();

  enum ParserState {
    PROTOCOL_STATE_CONTROL,
    PROTOCOL_STATE_CHUNK
  };
  ParserState mState;

  enum ChunkType {
    CHUNK_ADD,
    CHUNK_SUB
  };

  struct ChunkState {
    ChunkType type;
    uint32 num;
    uint32 hashSize;
    uint32 length;
    void Clear() { num = 0; hashSize = 0; length = 0; }
  };
  ChunkState mChunkState;

  uint32_t mHashKey;
  bool mPerClientRandomize;
  nsCOMPtr<nsICryptoHash> mCryptoHash;

  nsresult mUpdateStatus;
  nsCString mPending;

  nsCOMPtr<nsICryptoHMAC> mHMAC;
  nsCString mServerMAC;

  uint32 mUpdateWait;
  bool mResetRequested;
  bool mRekeyRequested;

  nsTArray<ForwardedUpdate> mForwards;
  nsTArray<TableUpdate*> mTableUpdates;
  TableUpdate *mTableUpdate;
};

}
}

#endif
