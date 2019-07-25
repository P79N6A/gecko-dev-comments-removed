





#ifndef nsDOMFileReaderSyncPrivate_h
#define nsDOMFileReaderSyncPrivate_h

#include "Workers.h"

#include "nsICharsetDetectionObserver.h"
#include "nsStringGlue.h"
#include "mozilla/Attributes.h"

class nsIInputStream;
class nsIDOMBlob;

BEGIN_WORKERS_NAMESPACE

class FileReaderSyncPrivate MOZ_FINAL : public PrivatizableBase,
                                        public nsICharsetDetectionObserver
{
  nsCString mCharset;
  nsresult ConvertStream(nsIInputStream *aStream, const char *aCharset,
                         nsAString &aResult);
  nsresult GuessCharset(nsIInputStream *aStream, nsACString &aCharset);

public:
  NS_DECL_ISUPPORTS

  FileReaderSyncPrivate();
  ~FileReaderSyncPrivate();

  nsresult ReadAsArrayBuffer(nsIDOMBlob* aBlob, uint32_t aLength,
                             uint8* aBuffer);
  nsresult ReadAsBinaryString(nsIDOMBlob* aBlob, nsAString& aResult);
  nsresult ReadAsText(nsIDOMBlob* aBlob, const nsAString& aEncoding,
                      nsAString& aResult);
  nsresult ReadAsDataURL(nsIDOMBlob* aBlob, nsAString& aResult);

  
  NS_IMETHOD Notify(const char *aCharset, nsDetectionConfident aConf);
};

END_WORKERS_NAMESPACE

#endif
