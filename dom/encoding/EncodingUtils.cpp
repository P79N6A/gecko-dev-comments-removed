



#include "mozilla/dom/EncodingUtils.h"
#include "nsUConvPropertySearch.h"

namespace mozilla {
namespace dom {

static const char* labelsEncodings[][3] = {
#include "labelsencodings.properties.h"
};

uint32_t
EncodingUtils::IdentifyDataOffset(const char* aData,
                                  const uint32_t aLength,
                                  nsACString& aRetval)
{
  
  aRetval.Truncate();

  
  
  if (aLength < 2) {
    return 0;
  }

  if (aData[0] == '\xFF' && aData[1] == '\xFE') {
    aRetval.AssignLiteral("UTF-16LE");
    return 2;
  }

  if (aData[0] == '\xFE' && aData[1] == '\xFF') {
    aRetval.AssignLiteral("UTF-16BE");
    return 2;
  }

  
  
  
  if (aLength < 3) {
    return 0;
  }

  if (aData[0] == '\xEF' && aData[1] == '\xBB' && aData[2] == '\xBF') {
    aRetval.AssignLiteral("UTF-8");
    return 3;
  }
  return 0;
}

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
EncodingUtils::IsAsciiCompatible(const nsACString& aPreferredName)
{
  return !(aPreferredName.LowerCaseEqualsLiteral("utf-16") ||
           aPreferredName.LowerCaseEqualsLiteral("utf-16be") ||
           aPreferredName.LowerCaseEqualsLiteral("utf-16le") ||
           aPreferredName.LowerCaseEqualsLiteral("utf-7") ||
           aPreferredName.LowerCaseEqualsLiteral("x-imap4-modified-utf7"));
}

} 
} 
