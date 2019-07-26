




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
#include "nsCycleCollectionParticipant.h"

class nsIExpatSink;
class nsIExtendedExpatSink;
struct nsCatalogData;

class nsExpatDriver : public nsIDTD,
                      public nsITokenizer
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDTD
  NS_DECL_NSITOKENIZER
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsExpatDriver, nsIDTD)

  nsExpatDriver();
  virtual ~nsExpatDriver();

  int HandleExternalEntityRef(const PRUnichar *aOpenEntityNames,
                              const PRUnichar *aBase,
                              const PRUnichar *aSystemId,
                              const PRUnichar *aPublicId);
  nsresult HandleStartElement(const PRUnichar *aName, const PRUnichar **aAtts);
  nsresult HandleEndElement(const PRUnichar *aName);
  nsresult HandleCharacterData(const PRUnichar *aCData, const uint32_t aLength);
  nsresult HandleComment(const PRUnichar *aName);
  nsresult HandleProcessingInstruction(const PRUnichar *aTarget,
                                       const PRUnichar *aData);
  nsresult HandleXMLDeclaration(const PRUnichar *aVersion,
                                const PRUnichar *aEncoding,
                                int32_t aStandalone);
  nsresult HandleDefault(const PRUnichar *aData, const uint32_t aLength);
  nsresult HandleStartCdataSection();
  nsresult HandleEndCdataSection();
  nsresult HandleStartDoctypeDecl(const PRUnichar* aDoctypeName,
                                  const PRUnichar* aSysid,
                                  const PRUnichar* aPubid,
                                  bool aHasInternalSubset);
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
  nsresult HandleToken(CToken* aToken);

  
  nsresult OpenInputStreamFromExternalDTD(const PRUnichar* aFPIStr,
                                          const PRUnichar* aURLStr,
                                          const PRUnichar* aBaseURL,
                                          nsIInputStream** aStream,
                                          nsAString& aAbsURL);

  















  void ParseBuffer(const PRUnichar *aBuffer, uint32_t aLength, bool aIsFinal,
                   uint32_t *aConsumed);
  nsresult HandleError();

  void MaybeStopParser(nsresult aState);

  bool BlockedOrInterrupted()
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
  bool             mInCData;
  bool             mInInternalSubset;
  bool             mInExternalDTD;
  bool             mMadeFinalCallToExpat;

  
  
  bool             mIsFinalChunk;

  nsresult         mInternalState;

  
  uint32_t         mExpatBuffered;

  
  
  
  nsCOMPtr<nsIContentSink> mOriginalSink;
  nsCOMPtr<nsIExpatSink> mSink;
  nsCOMPtr<nsIExtendedExpatSink> mExtendedSink;

  const nsCatalogData* mCatalogData; 
  nsString         mURISpec;

  
  uint64_t         mInnerWindowID;
};

#endif
