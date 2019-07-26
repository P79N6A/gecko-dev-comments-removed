




#ifndef nsSaveAsCharset_h__
#define nsSaveAsCharset_h__

#include "nsStringFwd.h"
#include "nsTArray.h"
#include "nsISaveAsCharset.h"
#include "nsCOMPtr.h"

#define MASK_FALLBACK(a) (nsISaveAsCharset::mask_Fallback & (a))
#define MASK_ENTITY(a) (nsISaveAsCharset::mask_Entity & (a))
#define MASK_CHARSET_FALLBACK(a) (nsISaveAsCharset::mask_CharsetFallback & (a))
#define ATTR_NO_FALLBACK(a) (nsISaveAsCharset::attr_FallbackNone == MASK_FALLBACK(a) && \
                             nsISaveAsCharset::attr_EntityAfterCharsetConv != MASK_ENTITY(a))

class nsIUnicodeEncoder;
class nsIEntityConverter;

class nsSaveAsCharset : public nsISaveAsCharset
{
public:
	
	
	
	
  nsSaveAsCharset();
  virtual ~nsSaveAsCharset();

	
	
	
	NS_DECL_ISUPPORTS

	
	
	
  NS_IMETHOD Init(const char *charset, uint32_t attr, uint32_t entityVersion);

  NS_IMETHOD Convert(const char16_t *inString, char **_retval);

  NS_IMETHODIMP GetCharset(char * *aCharset);

protected:

  NS_IMETHOD DoCharsetConversion(const char16_t *inString, char **outString);

  NS_IMETHOD DoConversionFallBack(uint32_t inUCS4, char *outString, int32_t bufferLength);

  
  
  NS_IMETHOD HandleFallBack(uint32_t character, char **outString, int32_t *bufferLength, 
                            int32_t *currentPos, int32_t estimatedLength);

  nsresult SetupUnicodeEncoder(const char* charset);

  nsresult SetupCharsetList(const char *charsetList);

  const char * GetNextCharset();

  uint32_t mAttribute;                    
  uint32_t mEntityVersion;                
  nsCOMPtr<nsIUnicodeEncoder> mEncoder;   
  nsCOMPtr<nsIEntityConverter> mEntityConverter;
  nsTArray<nsCString> mCharsetList;
  int32_t        mCharsetListIndex;
};

#endif
