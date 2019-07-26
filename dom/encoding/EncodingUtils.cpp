



#include "mozilla/dom/EncodingUtils.h"

#include "mozilla/ArrayUtils.h" 
#include "nsUConvPropertySearch.h"
#include "nsIUnicodeDecoder.h"
#include "nsIUnicodeEncoder.h"
#include "nsComponentManagerUtils.h"

namespace mozilla {
namespace dom {

static const char* labelsEncodings[][3] = {
#include "labelsencodings.properties.h"
};

static const char* encodingsGroups[][3] = {
#include "encodingsgroups.properties.h"
};

bool
EncodingUtils::FindEncodingForLabel(const nsACString& aLabel,
                                    nsACString& aOutEncoding)
{
  
  nsCString label(aLabel);

  EncodingUtils::TrimSpaceCharacters(label);
  if (label.IsEmpty()) {
    aOutEncoding.Truncate();
    return false;
  }

  ToLowerCase(label);
  return NS_SUCCEEDED(nsUConvPropertySearch::SearchPropertyValue(
      labelsEncodings, ArrayLength(labelsEncodings), label, aOutEncoding));
}

bool
EncodingUtils::FindEncodingForLabelNoReplacement(const nsACString& aLabel,
                                                 nsACString& aOutEncoding)
{
  if(!FindEncodingForLabel(aLabel, aOutEncoding)) {
    return false;
  }
  if (aOutEncoding.EqualsLiteral("replacement")) {
    aOutEncoding.Truncate();
    return false;
  }
  return true;
}

bool
EncodingUtils::IsAsciiCompatible(const nsACString& aPreferredName)
{
  return !(aPreferredName.LowerCaseEqualsLiteral("utf-16") ||
           aPreferredName.LowerCaseEqualsLiteral("utf-16be") ||
           aPreferredName.LowerCaseEqualsLiteral("utf-16le") ||
           aPreferredName.LowerCaseEqualsLiteral("replacement") ||
           aPreferredName.LowerCaseEqualsLiteral("hz-gb-2312") ||
           aPreferredName.LowerCaseEqualsLiteral("utf-7") ||
           aPreferredName.LowerCaseEqualsLiteral("x-imap4-modified-utf7"));
}

already_AddRefed<nsIUnicodeDecoder>
EncodingUtils::DecoderForEncoding(const nsACString& aEncoding)
{
  nsAutoCString contractId(NS_UNICODEDECODER_CONTRACTID_BASE);
  contractId.Append(aEncoding);

  nsCOMPtr<nsIUnicodeDecoder> decoder = do_CreateInstance(contractId.get());
  MOZ_ASSERT(decoder, "Tried to create decoder for unknown encoding.");
  return decoder.forget();
}

already_AddRefed<nsIUnicodeEncoder>
EncodingUtils::EncoderForEncoding(const nsACString& aEncoding)
{
  nsAutoCString contractId(NS_UNICODEENCODER_CONTRACTID_BASE);
  contractId.Append(aEncoding);

  nsCOMPtr<nsIUnicodeEncoder> encoder = do_CreateInstance(contractId.get());
  MOZ_ASSERT(encoder, "Tried to create encoder for unknown encoding.");
  return encoder.forget();
}

void
EncodingUtils::LangGroupForEncoding(const nsACString& aEncoding,
                                    nsACString& aOutGroup)
{
  if (NS_FAILED(nsUConvPropertySearch::SearchPropertyValue(
      encodingsGroups, ArrayLength(encodingsGroups), aEncoding, aOutGroup))) {
    aOutGroup.AssignLiteral("x-unicode");
  }
}

} 
} 
