




































#ifndef NS_EXPAT_DRIVER__
#define NS_EXPAT_DRIVER__

#include "expat_config.h"
#include "expat.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIDTD.h"
#include "nsITokenizer.h"
#include "nsIInputStream.h"
#include "nsIParser.h"

class nsIExpatSink;
class nsIExtendedExpatSink;
struct nsCatalogData;

class nsExpatDriver : public nsIDTD,
                      public nsITokenizer
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDTD
  NS_DECL_NSITOKENIZER

  nsExpatDriver();
  virtual ~nsExpatDriver();

  int HandleExternalEntityRef(const PRUnichar *aOpenEntityNames,
                              const PRUnichar *aBase,
                              const PRUnichar *aSystemId,
                              const PRUnichar *aPublicId);
  nsresult HandleStartElement(const PRUnichar *aName, const PRUnichar **aAtts);
  nsresult HandleEndElement(const PRUnichar *aName);
  nsresult HandleCharacterData(const PRUnichar *aCData, const PRUint32 aLength);
  nsresult HandleComment(const PRUnichar *aName);
  nsresult HandleProcessingInstruction(const PRUnichar *aTarget,
                                       const PRUnichar *aData);
  nsresult HandleXMLDeclaration(const PRUnichar *aVersion,
                                const PRUnichar *aEncoding,
                                PRInt32 aStandalone);
  nsresult HandleDefault(const PRUnichar *aData, const PRUint32 aLength);
  nsresult HandleStartCdataSection();
  nsresult HandleEndCdataSection();
  nsresult HandleStartDoctypeDecl(const PRUnichar* aDoctypeName,
                                  const PRUnichar* aSysid,
                                  const PRUnichar* aPubid,
                                  PRBool aHasInternalSubset);
  nsresult HandleEndDoctypeDecl();
  nsresult HandleStartNamespaceDecl(const PRUnichar* aPrefix,
                                    const PRUnichar* aUri);
  nsresult HandleEndNamespaceDecl(const PRUnichar* aPrefix);
  nsresult HandleNotationDecl(const PRUnichar* aNotationName,
                              const PRUnichar* aBase,
                              const PRUnichar* aSysid,
                              const PRUnichar* aPubid);
  nsresult HandleUnparsedEntityDecl(const PRUnichar* aEntityName,
                                    const PRUnichar* aBase,
                                    const PRUnichar* aSysid,
                                    const PRUnichar* aPubid,
                                    const PRUnichar* aNotationName);

private:
  
  nsresult OpenInputStreamFromExternalDTD(const PRUnichar* aFPIStr,
                                          const PRUnichar* aURLStr,
                                          const PRUnichar* aBaseURL,
                                          nsIInputStream** aStream,
                                          nsAString& aAbsURL);

  















  void ParseBuffer(const PRUnichar *aBuffer, PRUint32 aLength, PRBool aIsFinal,
                   PRUint32 *aConsumed);
  nsresult HandleError();

  void MaybeStopParser();

  PRBool BlockedOrInterrupted()
  {
    return mInternalState == NS_ERROR_HTMLPARSER_BLOCK ||
           mInternalState == NS_ERROR_HTMLPARSER_INTERRUPTED;
  }

  XML_Parser       mExpatParser;
  nsString         mLastLine;
  nsString         mCDataText;
  
  nsString         mDoctypeName;
  nsString         mSystemID;
  nsString         mPublicID;
  nsString         mInternalSubset;
  PRPackedBool     mInCData;
  PRPackedBool     mInInternalSubset;
  PRPackedBool     mInExternalDTD;

  
  
  PRPackedBool     mIsFinalChunk;

  nsresult         mInternalState;

  
  PRUint32         mExpatBuffered;

  nsCOMPtr<nsIExpatSink> mSink;
  nsCOMPtr<nsIExtendedExpatSink> mExtendedSink;
  const nsCatalogData* mCatalogData; 
  nsString         mURISpec;
};

#endif
