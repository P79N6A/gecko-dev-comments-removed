



#ifndef mozilla_dom_encodingutils_h_
#define mozilla_dom_encodingutils_h_

#include "nsDataHashtable.h"
#include "nsString.h"

class nsIUnicodeDecoder;
class nsIUnicodeEncoder;

namespace mozilla {
namespace dom {

class EncodingUtils
{
public:

  












  static bool FindEncodingForLabel(const nsACString& aLabel,
                                   nsACString& aOutEncoding);

  static bool FindEncodingForLabel(const nsAString& aLabel,
                                   nsACString& aOutEncoding)
  {
    return FindEncodingForLabel(NS_ConvertUTF16toUTF8(aLabel), aOutEncoding);
  }

  








  static bool FindEncodingForLabelNoReplacement(const nsACString& aLabel,
                                                nsACString& aOutEncoding);

  static bool FindEncodingForLabelNoReplacement(const nsAString& aLabel,
                                                nsACString& aOutEncoding)
  {
    return FindEncodingForLabelNoReplacement(NS_ConvertUTF16toUTF8(aLabel),
                                             aOutEncoding);
  }

  









  template<class T>
  static void TrimSpaceCharacters(T& aString)
  {
    aString.Trim(" \t\n\f\r");
  }

  






  static bool IsAsciiCompatible(const nsACString& aPreferredName);

  





  static already_AddRefed<nsIUnicodeDecoder>
  DecoderForEncoding(const char* aEncoding)
  {
    nsDependentCString encoding(aEncoding);
    return DecoderForEncoding(encoding);
  }

  





  static already_AddRefed<nsIUnicodeDecoder>
  DecoderForEncoding(const nsACString& aEncoding);

  





  static already_AddRefed<nsIUnicodeEncoder>
  EncoderForEncoding(const char* aEncoding)
  {
    nsDependentCString encoding(aEncoding);
    return EncoderForEncoding(encoding);
  }

  





  static already_AddRefed<nsIUnicodeEncoder>
  EncoderForEncoding(const nsACString& aEncoding);

  






  static void LangGroupForEncoding(const nsACString& aEncoding,
                                   nsACString& aOutGroup);

private:
  EncodingUtils() MOZ_DELETE;
};

} 
} 

#endif 
