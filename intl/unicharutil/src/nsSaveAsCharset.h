




































#include "nsIFactory.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "nsICharsetConverterManager.h"
#include "nsISaveAsCharset.h"


#define MASK_FALLBACK(a) (nsISaveAsCharset::mask_Fallback & (a))
#define MASK_ENTITY(a) (nsISaveAsCharset::mask_Entity & (a))
#define MASK_CHARSET_FALLBACK(a) (nsISaveAsCharset::mask_CharsetFallback & (a))
#define MASK_IGNORABLE_FALLBACK(a) (nsISaveAsCharset::mask_IgnorableFallback & (a))
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

	
	
	
  NS_IMETHOD Init(const char *charset, PRUint32 attr, PRUint32 entityVersion);

  NS_IMETHOD Convert(const PRUnichar *inString, char **_retval);

  NS_IMETHODIMP GetCharset(char * *aCharset);

protected:

  NS_IMETHOD DoCharsetConversion(const PRUnichar *inString, char **outString);

  NS_IMETHOD DoConversionFallBack(PRUint32 inUCS4, char *outString, PRInt32 bufferLength);

  
  
  NS_IMETHOD HandleFallBack(PRUint32 character, char **outString, PRInt32 *bufferLength, 
                            PRInt32 *currentPos, PRInt32 estimatedLength);

  nsresult SetupUnicodeEncoder(const char* charset);

  nsresult SetupCharsetList(const char *charsetList);

  const char * GetNextCharset();

  PRUint32 mAttribute;                    
  PRUint32 mEntityVersion;                
  nsCOMPtr<nsIUnicodeEncoder> mEncoder;   
  nsCOMPtr<nsIEntityConverter> mEntityConverter;
  nsCStringArray mCharsetList;
  PRInt32        mCharsetListIndex;
};


nsresult NS_NewSaveAsCharset(nsISupports **inst);

