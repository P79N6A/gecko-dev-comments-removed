





































#ifndef nsJSON_h__
#define nsJSON_h__

#include "jsapi.h"
#include "json.h"
#include "nsIJSON.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIOutputStream.h"
#include "nsIUnicodeEncoder.h"
#include "nsIUnicodeDecoder.h"
#include "nsIRequestObserver.h"
#include "nsIStreamListener.h"
#include "nsTArray.h"

class nsIURI;

class NS_STACK_CLASS nsJSONWriter
{
public:
  nsJSONWriter();
  nsJSONWriter(nsIOutputStream *aStream);
  virtual ~nsJSONWriter();
  nsresult SetCharset(const char *aCharset);
  nsCOMPtr<nsIOutputStream> mStream;
  nsresult Write(const PRUnichar *aBuffer, PRUint32 aLength);
  nsString mOutputString;
  PRBool DidWrite();
  void FlushBuffer();

protected:
  PRUnichar *mBuffer;
  PRUint32 mBufferCount;
  PRBool mDidWrite;
  nsresult WriteToStream(nsIOutputStream *aStream, nsIUnicodeEncoder *encoder,
                         const PRUnichar *aBuffer, PRUint32 aLength);

  nsCOMPtr<nsIUnicodeEncoder> mEncoder;
};

class nsJSON : public nsIJSON
{
public:
  nsJSON();
  virtual ~nsJSON();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIJSON

protected:
  nsresult EncodeInternal(nsJSONWriter *writer);

  nsresult DecodeInternal(nsIInputStream *aStream,
                          PRInt32 aContentLength,
                          PRBool aNeedsConverter,
                          DecodingMode mode = STRICT);
  nsCOMPtr<nsIURI> mURI;
};

nsresult
NS_NewJSON(nsISupports* aOuter, REFNSIID aIID, void** aResult);

class nsJSONListener : public nsIStreamListener
{
public:
  nsJSONListener(JSContext *cx, jsval *rootVal, PRBool needsConverter,
                 DecodingMode mode);
  virtual ~nsJSONListener();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

protected:
  PRBool mNeedsConverter;
  JSContext *mCx;
  jsval *mRootVal;
  nsCOMPtr<nsIUnicodeDecoder> mDecoder;
  nsCString mSniffBuffer;
  nsTArray<PRUnichar> mBufferedChars;
  DecodingMode mDecodingMode;
  nsresult ProcessBytes(const char* aBuffer, PRUint32 aByteLength);
  nsresult ConsumeConverted(const char* aBuffer, PRUint32 aByteLength);
  nsresult Consume(const PRUnichar *data, PRUint32 len);
};

#endif
