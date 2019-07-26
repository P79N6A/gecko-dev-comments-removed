



#ifndef mozilla_dom_encodingutils_h_
#define mozilla_dom_encodingutils_h_

#include "nsDataHashtable.h"
#include "nsString.h"

namespace mozilla {
namespace dom {

class EncodingUtils
{
public:
  NS_INLINE_DECL_REFCOUNTING(EncodingUtils)

  










  static uint32_t IdentifyDataOffset(const char* aData,
                                     const uint32_t aLength,
                                     const char*& aRetval);

  










  static bool FindEncodingForLabel(const nsAString& aLabel,
                                   const char*& aOutEncoding);

  









  static void TrimSpaceCharacters(nsString& aString)
  {
    aString.Trim(" \t\n\f\r");
  }

  
  static void Shutdown();

protected:
  nsDataHashtable<nsStringHashKey, const char *> mLabelsEncodings;
  EncodingUtils();
  virtual ~EncodingUtils();
  static already_AddRefed<EncodingUtils> GetOrCreate();
};

} 
} 

#endif 
