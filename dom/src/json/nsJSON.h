





































#ifndef nsJSON_h__
#define nsJSON_h__

#include "jsprvtd.h"
#include "nsIJSON.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIOutputStream.h"
#include "nsIUnicodeEncoder.h"
#include "nsIUnicodeDecoder.h"
#include "nsIRequestObserver.h"
#include "nsIStreamListener.h"
#include "nsTArray.h"

#define JSON_MAX_DEPTH  2048
#define JSON_PARSER_BUFSIZE 1024
class nsJSONWriter
{
public:
  nsJSONWriter();
  nsJSONWriter(nsIOutputStream *aStream);
  virtual ~nsJSONWriter();
  nsresult SetCharset(const char *aCharset);
  nsString mBuffer;
  nsCOMPtr<nsIOutputStream> mStream;
  nsresult WriteString(const PRUnichar* aBuffer, PRUint32);
  nsresult Write(const PRUnichar *aBuffer, PRUint32 aLength);

protected:
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
  JSBool   ToJSON(JSContext *cx, jsval *vp);
  nsresult EncodeObject(JSContext *cx, jsval *vp, nsJSONWriter *writer,
                        JSObject *whitelist, PRUint32 depth);
  nsresult EncodeInternal(nsJSONWriter *writer);
  nsresult DecodeInternal(nsIInputStream *aStream,
                          PRInt32 aContentLength,
                          PRBool aNeedsConverter);
};

NS_IMETHODIMP
NS_NewJSON(nsISupports* aOuter, REFNSIID aIID, void** aResult);

enum JSONParserState {
    JSON_PARSE_STATE_INIT,
    JSON_PARSE_STATE_VALUE,
    JSON_PARSE_STATE_OBJECT,
    JSON_PARSE_STATE_OBJECT_PAIR,
    JSON_PARSE_STATE_OBJECT_IN_PAIR,
    JSON_PARSE_STATE_ARRAY,
    JSON_PARSE_STATE_STRING,
    JSON_PARSE_STATE_STRING_ESCAPE,
    JSON_PARSE_STATE_STRING_HEX,
    JSON_PARSE_STATE_NUMBER,
    JSON_PARSE_STATE_KEYWORD,
    JSON_PARSE_STATE_FINISHED
};

class nsJSONObjectStack : public nsTArray<JSObject *>,
                          public JSTempValueRooter
{
};

class nsJSONListener : public nsIStreamListener
{
public:
  nsJSONListener(JSContext *cx, jsval *rootVal, PRBool needsConverter);
  virtual ~nsJSONListener();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

protected:
  PRUint32 mLineNum;
  PRUint32 mColumn;

  
  PRUnichar mHexChar;
  PRUint8 mNumHex;

  JSContext *mCx;
  jsval *mRootVal;
  PRBool mNeedsConverter;
  nsCOMPtr<nsIUnicodeDecoder> mDecoder;
  JSONParserState *mStatep;
  JSONParserState mStateStack[JSON_MAX_DEPTH];
  nsString mStringBuffer;
  nsCString mSniffBuffer;

  nsresult PushState(JSONParserState state);
  nsresult PopState();
  nsresult ProcessBytes(const char* aBuffer, PRUint32 aByteLength);
  nsresult ConsumeConverted(const char* aBuffer, PRUint32 aByteLength);
  nsresult Consume(const PRUnichar *data, PRUint32 len);

  
  nsJSONObjectStack mObjectStack;

  nsresult PushValue(JSObject *aParent, jsval aValue);
  nsresult PushObject(JSObject *aObj);
  nsresult OpenObject();
  nsresult CloseObject();
  nsresult OpenArray();
  nsresult CloseArray();
  nsresult HandleString();
  nsresult HandleNumber();
  nsresult HandleKeyword();
  nsString mObjectKey;
};

#endif
