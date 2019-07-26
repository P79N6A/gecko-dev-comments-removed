



#ifndef mozilla_dom_encodingutils_h_
#define mozilla_dom_encodingutils_h_

#include "nsDataHashtable.h"
#include "nsString.h"

namespace mozilla {
namespace dom {

class EncodingUtils
{
public:

  












  static uint32_t IdentifyDataOffset(const char* aData,
                                     const uint32_t aLength,
                                     nsACString& aRetval);

  












  static bool FindEncodingForLabel(const nsACString& aLabel,
                                   nsACString& aOutEncoding);

  static bool FindEncodingForLabel(const nsAString& aLabel,
                                   nsACString& aOutEncoding)
  {
    return FindEncodingForLabel(NS_ConvertUTF16toUTF8(aLabel), aOutEncoding);
  }

  









  template<class T>
  static void TrimSpaceCharacters(T& aString)
  {
    aString.Trim(" \t\n\f\r");
  }

  






  static bool IsAsciiCompatible(const nsACString& aPreferredName);

private:
  EncodingUtils() MOZ_DELETE;
};

} 
} 

#endif 
