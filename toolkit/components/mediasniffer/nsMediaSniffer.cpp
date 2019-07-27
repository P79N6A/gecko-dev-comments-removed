





#include "nsMediaSniffer.h"
#include "nsIHttpChannel.h"
#include "nsString.h"
#include "nsMimeTypes.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/ModuleUtils.h"
#include "mp3sniff.h"
#ifdef MOZ_WEBM
#include "nestegg/nestegg.h"
#endif

#include "nsIClassInfoImpl.h"
#include <algorithm>


static const unsigned MP4_MIN_BYTES_COUNT = 12;

static const uint32_t MAX_BYTES_SNIFFED = 512;



static const uint32_t MAX_BYTES_SNIFFED_MP3 = 320 * 144 / 32 + 1 + 4;

NS_IMPL_ISUPPORTS(nsMediaSniffer, nsIContentSniffer)

nsMediaSniffer::nsMediaSnifferEntry nsMediaSniffer::sSnifferEntries[] = {
  
  PATTERN_ENTRY("\xFF\xFF\xFF\xFF\xFF", "OggS", APPLICATION_OGG),
  
  PATTERN_ENTRY("\xFF\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF", "RIFF\x00\x00\x00\x00WAVE", AUDIO_WAV),
  
  PATTERN_ENTRY("\xFF\xFF\xFF", "ID3", AUDIO_MP3)
};



static bool MatchesMP4(const uint8_t* aData, const uint32_t aLength)
{
  if (aLength <= MP4_MIN_BYTES_COUNT) {
    return false;
  }
  
  uint32_t boxSize = (uint32_t)(aData[3] | aData[2] << 8 | aData[1] << 16 | aData[0] << 24);

  
  if (boxSize % 4 || aLength < boxSize) {
    return false;
  }
  
  if (aData[4] != 0x66 ||
      aData[5] != 0x74 ||
      aData[6] != 0x79 ||
      aData[7] != 0x70) {
    return false;
  }
  for (uint32_t i = 2; i <= boxSize / 4 - 1 ; i++) {
    if (i == 3) {
      continue;
    }
    
    if (aData[4*i]   == 0x6D &&
        aData[4*i+1] == 0x70 &&
        aData[4*i+2] == 0x34) {
      return true;
    }
    
    if (aData[4*i]   == 0x69 &&
        aData[4*i+1] == 0x73 &&
        aData[4*i+2] == 0x6F &&
        (aData[4*i+3] == 0x6D || aData[4*i+3] == 0x32)) {
      return true;
    }
  }
  return false;
}

static bool MatchesWebM(const uint8_t* aData, const uint32_t aLength)
{
#ifdef MOZ_WEBM
  return nestegg_sniff((uint8_t*)aData, aLength) ? true : false;
#else
  return false;
#endif
}



static bool MatchesMP3(const uint8_t* aData, const uint32_t aLength)
{
  return mp3_sniff(aData, (long)aLength);
}

NS_IMETHODIMP
nsMediaSniffer::GetMIMETypeFromContent(nsIRequest* aRequest,
                                       const uint8_t* aData,
                                       const uint32_t aLength,
                                       nsACString& aSniffedType)
{
  
  
  nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest);
  if (channel) {
    nsAutoCString contentType;
    nsresult rv = channel->GetContentType(contentType);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!contentType.IsEmpty() &&
        !contentType.EqualsLiteral(APPLICATION_OCTET_STREAM) &&
        !contentType.EqualsLiteral(UNKNOWN_CONTENT_TYPE)) {
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  const uint32_t clampedLength = std::min(aLength, MAX_BYTES_SNIFFED);

  for (uint32_t i = 0; i < mozilla::ArrayLength(sSnifferEntries); ++i) {
    const nsMediaSnifferEntry& currentEntry = sSnifferEntries[i];
    if (clampedLength < currentEntry.mLength || currentEntry.mLength == 0) {
      continue;
    }
    bool matched = true;
    for (uint32_t j = 0; j < currentEntry.mLength; ++j) {
      if ((currentEntry.mMask[j] & aData[j]) != currentEntry.mPattern[j]) {
        matched = false;
        break;
      }
    }
    if (matched) {
      aSniffedType.AssignASCII(currentEntry.mContentType);
      return NS_OK;
    }
  }

  if (MatchesMP4(aData, clampedLength)) {
    aSniffedType.AssignLiteral(VIDEO_MP4);
    return NS_OK;
  }

  if (MatchesWebM(aData, clampedLength)) {
    aSniffedType.AssignLiteral(VIDEO_WEBM);
    return NS_OK;
  }

  
  if (MatchesMP3(aData, std::min(aLength, MAX_BYTES_SNIFFED_MP3))) {
    aSniffedType.AssignLiteral(AUDIO_MP3);
    return NS_OK;
  }

  
  
  aSniffedType.AssignLiteral(APPLICATION_OCTET_STREAM);
  return NS_ERROR_NOT_AVAILABLE;
}
