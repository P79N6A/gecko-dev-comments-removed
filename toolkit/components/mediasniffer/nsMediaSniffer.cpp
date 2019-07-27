





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

nsMediaSnifferEntry nsMediaSniffer::sSnifferEntries[] = {
  
  PATTERN_ENTRY("\xFF\xFF\xFF\xFF\xFF", "OggS", APPLICATION_OGG),
  
  PATTERN_ENTRY("\xFF\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF", "RIFF\x00\x00\x00\x00WAVE", AUDIO_WAV),
  
  PATTERN_ENTRY("\xFF\xFF\xFF", "ID3", AUDIO_MP3)
};


nsMediaSnifferEntry sFtypEntries[] = {
  PATTERN_ENTRY("\xFF\xFF\xFF", "mp4", VIDEO_MP4), 
  PATTERN_ENTRY("\xFF\xFF\xFF", "3gp", VIDEO_3GPP), 
  PATTERN_ENTRY("\xFF\xFF\xFF\xFF", "M4A ", AUDIO_MP4),
  PATTERN_ENTRY("\xFF\xFF\xFF\xFF", "M4P ", AUDIO_MP4)
};

static bool MatchesBrands(const uint8_t aData[4], nsACString& aSniffedType)
{
  for (size_t i = 0; i < mozilla::ArrayLength(sFtypEntries); ++i) {
    const auto& currentEntry = sFtypEntries[i];
    bool matched = true;
    MOZ_ASSERT(currentEntry.mLength <= 4, "Pattern is too large to match brand strings.");
    for (uint32_t j = 0; j < currentEntry.mLength; ++j) {
      if ((currentEntry.mMask[j] & aData[j]) != currentEntry.mPattern[j]) {
        matched = false;
        break;
      }
    }
    if (matched) {
      aSniffedType.AssignASCII(currentEntry.mContentType);
      return true;
    }
  }

  return false;
}




static bool MatchesMP4(const uint8_t* aData, const uint32_t aLength, nsACString& aSniffedType)
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
  if (MatchesBrands(&aData[8], aSniffedType)) {
    return true;
  }
  
  uint32_t bytesRead = 16;
  while (bytesRead < boxSize) {
    if (MatchesBrands(&aData[bytesRead], aSniffedType)) {
      return true;
    }
    bytesRead += 4;
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
    nsLoadFlags loadFlags = 0;
    channel->GetLoadFlags(&loadFlags);
    if (!(loadFlags & nsIChannel::LOAD_MEDIA_SNIFFER_OVERRIDES_CONTENT_TYPE)) {
      
      
      nsAutoCString contentType;
      nsresult rv = channel->GetContentType(contentType);
      NS_ENSURE_SUCCESS(rv, rv);
      if (!contentType.IsEmpty() &&
          !contentType.EqualsLiteral(APPLICATION_OCTET_STREAM) &&
          !contentType.EqualsLiteral(UNKNOWN_CONTENT_TYPE)) {
        return NS_ERROR_NOT_AVAILABLE;
      }
    }
  }

  const uint32_t clampedLength = std::min(aLength, MAX_BYTES_SNIFFED);

  for (size_t i = 0; i < mozilla::ArrayLength(sSnifferEntries); ++i) {
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

  if (MatchesMP4(aData, clampedLength, aSniffedType)) {
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
