



#ifndef nsIFormSubmission_h___
#define nsIFormSubmission_h___

#include "mozilla/Attributes.h"
#include "nsString.h"
#include "nsCOMPtr.h"

class nsIURI;
class nsIInputStream;
class nsGenericHTMLElement;
class nsIContent;
class nsISaveAsCharset;
class nsIMultiplexInputStream;

namespace mozilla {
namespace dom {
class File;
} 
} 





class nsFormSubmission
{
public:
  virtual ~nsFormSubmission()
  {
    MOZ_COUNT_DTOR(nsFormSubmission);
  }

  





  virtual nsresult AddNameValuePair(const nsAString& aName,
                                    const nsAString& aValue) = 0;

  





  virtual nsresult AddNameFilePair(const nsAString& aName,
                                   mozilla::dom::File* aBlob) = 0;

  




  virtual bool SupportsIsindexSubmission()
  {
    return false;
  }

  




  virtual nsresult AddIsindex(const nsAString& aValue)
  {
    NS_NOTREACHED("AddIsindex called when not supported");
    return NS_ERROR_UNEXPECTED;
  }

  






  virtual nsresult GetEncodedSubmission(nsIURI* aURI,
                                        nsIInputStream** aPostDataStream) = 0;

  


  void GetCharset(nsACString& aCharset)
  {
    aCharset = mCharset;
  }

  nsIContent* GetOriginatingElement() const
  {
    return mOriginatingElement.get();
  }

protected:
  





  nsFormSubmission(const nsACString& aCharset, nsIContent* aOriginatingElement)
    : mCharset(aCharset)
    , mOriginatingElement(aOriginatingElement)
  {
    MOZ_COUNT_CTOR(nsFormSubmission);
  }

  
  nsCString mCharset;

  
  nsCOMPtr<nsIContent> mOriginatingElement;
};

class nsEncodingFormSubmission : public nsFormSubmission
{
public:
  nsEncodingFormSubmission(const nsACString& aCharset,
                           nsIContent* aOriginatingElement);

  virtual ~nsEncodingFormSubmission();

  








  nsresult EncodeVal(const nsAString& aStr, nsCString& aResult,
                     bool aHeaderEncode);

private:
  
  nsCOMPtr<nsISaveAsCharset> mEncoder;
};





class nsFSMultipartFormData : public nsEncodingFormSubmission
{
public:
  


  nsFSMultipartFormData(const nsACString& aCharset,
                        nsIContent* aOriginatingElement);
  ~nsFSMultipartFormData();
 
  virtual nsresult AddNameValuePair(const nsAString& aName,
                                    const nsAString& aValue) override;
  virtual nsresult AddNameFilePair(const nsAString& aName,
                                   mozilla::dom::File* aBlob) override;
  virtual nsresult GetEncodedSubmission(nsIURI* aURI,
                                        nsIInputStream** aPostDataStream) override;

  void GetContentType(nsACString& aContentType)
  {
    aContentType =
      NS_LITERAL_CSTRING("multipart/form-data; boundary=") + mBoundary;
  }

  nsIInputStream* GetSubmissionBody(uint64_t* aContentLength);

protected:

  


  nsresult AddPostDataStream();

private:
  




  nsCOMPtr<nsIMultiplexInputStream> mPostDataStream;

  






  nsCString mPostDataChunk;

  




  nsCString mBoundary;

  


  uint64_t mTotalLength;
};








nsresult GetSubmissionFromForm(nsGenericHTMLElement* aForm,
                               nsGenericHTMLElement* aOriginatingElement,
                               nsFormSubmission** aFormSubmission);

#endif 
