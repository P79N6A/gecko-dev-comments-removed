




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
  virtual ~nsExpatDriver();

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDTD
  NS_DECL_NSITOKENIZER
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsExpatDriver, nsIDTD)

  nsExpatDriver();

  int HandleExternalEntityRef(const char16_t *aOpenEntityNames,
                              const char16_t *aBase,
                              const char16_t *aSystemId,
                              const char16_t *aPublicId);
  nsresult HandleStartElement(const char16_t *aName, const char16_t **aAtts);
  nsresult HandleEndElement(const char16_t *aName);
  nsresult HandleCharacterData(const char16_t *aCData, const uint32_t aLength);
  nsresult HandleComment(const char16_t *aName);
  nsresult HandleProcessingInstruction(const char16_t *aTarget,
                                       const char16_t *aData);
  nsresult HandleXMLDeclaration(const char16_t *aVersion,
                                const char16_t *aEncoding,
                                int32_t aStandalone);
  nsresult HandleDefault(const char16_t *aData, const uint32_t aLength);
  nsresult HandleStartCdataSection();
  nsresult HandleEndCdataSection();
  nsresult HandleStartDoctypeDecl(const char16_t* aDoctypeName,
                                  const char16_t* aSysid,
                                  const char16_t* aPubid,
                                  bool aHasInternalSubset);
  nsresult HandleEndDoctypeDecl();
  nsresult HandleStartNamespaceDecl(const char16_t* aPrefix,
                                    const char16_t* aUri);
  nsresult HandleEndNamespaceDecl(const char16_t* aPrefix);
  nsresult HandleNotationDecl(const char16_t* aNotationName,
                              const char16_t* aBase,
                              const char16_t* aSysid,
                              const char16_t* aPubid);
  nsresult HandleUnparsedEntityDecl(const char16_t* aEntityName,
                                    const char16_t* aBase,
                                    const char16_t* aSysid,
                                    const char16_t* aPubid,
                                    const char16_t* aNotationName);

private:
  
  nsresult OpenInputStreamFromExternalDTD(const char16_t* aFPIStr,
                                          const char16_t* aURLStr,
                                          const char16_t* aBaseURL,
                                          nsIInputStream** aStream,
                                          nsAString& aAbsURL);

  















  void ParseBuffer(const char16_t *aBuffer, uint32_t aLength, bool aIsFinal,
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
