




#ifndef ISOCOMPOSITOR_H_
#define ISOCOMPOSITOR_H_

#include "mozilla/Endian.h"
#include "nsTArray.h"
#include "ISOTrackMetadata.h"
#include "EncodedFrameContainer.h"

namespace mozilla {

class Box;
class ISOControl;









class FragmentBuffer {
public:
  
  
  
  FragmentBuffer(uint32_t aTrackType, uint32_t aFragDuration);
  ~FragmentBuffer();

  
  
  
  nsresult GetFirstFragment(nsTArray<nsRefPtr<EncodedFrame>>& aFragment,
                            bool aFlush = false);

  
  
  
  nsresult AddFrame(EncodedFrame* aFrame);

  
  uint32_t GetFirstFragmentSampleSize();

  
  uint32_t GetFirstFragmentSampleNumber();

  
  
  bool HasEnoughData();

  
  
  
  nsresult SetEndOfStream() {
    mEOS = true;
    return  NS_OK;
  }
  bool EOS() { return mEOS; }

  
  
  
  nsresult GetCSD(nsTArray<uint8_t>& aCSD);

  bool HasCSD() { return mCSDFrame; }

  uint32_t GetType() { return mTrackType; }

  void SetLastFragmentLastFrameTime(uint32_t aTime) {
    mLastFrameTimeOfLastFragment = aTime;
  }

  uint32_t GetLastFragmentLastFrameTime() {
    return mLastFrameTimeOfLastFragment;
  }

private:
  uint32_t mTrackType;

  
  uint32_t mFragDuration;

  
  
  
  
  
  uint64_t mMediaStartTime;

  
  
  
  
  
  uint32_t mFragmentNumber;

  
  
  
  
  uint32_t mLastFrameTimeOfLastFragment;

  
  
  nsTArray<nsTArray<nsRefPtr<EncodedFrame>>> mFragArray;

  
  
  
  nsRefPtr<EncodedFrame> mCSDFrame;

  
  bool mEOS;
};













class ISOControl {

friend class Box;

public:
  ISOControl(uint32_t aMuxingType);
  ~ISOControl();

  nsresult GenerateFtyp();
  nsresult GenerateMoov();
  nsresult GenerateMoof(uint32_t aTrackType);

  
  uint32_t WriteAVData(nsTArray<uint8_t>& aArray);

  uint32_t Write(uint8_t* aBuf, uint32_t aSize);

  uint32_t Write(uint8_t aData);

  template <typename T>
  uint32_t Write(T aData) {
    MOZ_ASSERT(!mBitCount);

    aData = NativeEndian::swapToNetworkOrder(aData);
    Write((uint8_t*)&aData, sizeof(T));
    return sizeof(T);
  }

  template <typename T>
  uint32_t WriteArray(const T &aArray, uint32_t aSize) {
    MOZ_ASSERT(!mBitCount);

    uint32_t size = 0;
    for (uint32_t i = 0; i < aSize; i++) {
      size += Write(aArray[i]);
    }
    return size;
  }

  uint32_t WriteFourCC(const char* aType);

  
  
  uint32_t WriteBits(uint64_t aBits, size_t aNumBits);

  
  nsresult GetBufs(nsTArray<nsTArray<uint8_t>>* aOutputBufs);

  
  uint32_t GetTime();

  
  uint32_t GetCurFragmentNumber() { return mFragNum; }

  nsresult SetFragment(FragmentBuffer* aFragment);
  FragmentBuffer* GetFragment(uint32_t aType);

  uint32_t GetMuxingType() { return mMuxingType; }

  nsresult SetMetadata(TrackMetadataBase* aTrackMeta);
  nsresult GetAudioMetadata(nsRefPtr<AudioTrackMetadata>& aAudMeta);
  nsresult GetVideoMetadata(nsRefPtr<VideoTrackMetadata>& aVidMeta);

  
  
  
  uint32_t GetTrackID(TrackMetadataBase::MetadataKind aKind);
  uint32_t GetNextTrackID();

  bool HasAudioTrack();
  bool HasVideoTrack();

private:
  uint32_t GetBufPos();
  nsresult FlushBuf();

  
  uint32_t mMuxingType;

  
  
  
  FragmentBuffer* mAudioFragmentBuffer;
  FragmentBuffer* mVideoFragmentBuffer;

  
  uint32_t mFragNum;

  
  nsTArray<nsRefPtr<TrackMetadataBase>> mMetaArray;

  
  
  
  
  
  
  
  
  
  
  
  
  
  nsTArray<nsTArray<uint8_t>> mOutBuffers;

  
  uint64_t mOutputSize;

  
  
  
  uint8_t mBitCount;
  uint8_t mBit;
};

}
#endif
