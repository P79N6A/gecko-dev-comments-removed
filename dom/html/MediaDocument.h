





#ifndef mozilla_dom_MediaDocument_h
#define mozilla_dom_MediaDocument_h

#include "mozilla/Attributes.h"
#include "nsHTMLDocument.h"
#include "nsGenericHTMLElement.h"
#include "nsAutoPtr.h"
#include "nsIStringBundle.h"

#define NSMEDIADOCUMENT_PROPERTIES_URI "chrome://global/locale/layout/MediaDocument.properties"

namespace mozilla {
namespace dom {

class MediaDocument : public nsHTMLDocument
{
public:
  MediaDocument();
  virtual ~MediaDocument();

  virtual nsresult Init() override;

  virtual nsresult StartDocumentLoad(const char*         aCommand,
                                     nsIChannel*         aChannel,
                                     nsILoadGroup*       aLoadGroup,
                                     nsISupports*        aContainer,
                                     nsIStreamListener** aDocListener,
                                     bool                aReset = true,
                                     nsIContentSink*     aSink = nullptr) override;

  virtual void SetScriptGlobalObject(nsIScriptGlobalObject* aGlobalObject) override;

  virtual bool WillIgnoreCharsetOverride() override
  {
    return true;
  }

protected:
  void BecomeInteractive();

  virtual nsresult CreateSyntheticDocument();

  friend class MediaDocumentStreamListener;
  nsresult StartLayout();

  void GetFileName(nsAString& aResult, nsIChannel* aChannel);

  nsresult LinkStylesheet(const nsAString& aStylesheet);

  
  
  
  
  
  
  
  
  
  
  
  void UpdateTitleAndCharset(const nsACString&  aTypeStr,
                             nsIChannel* aChannel,
                             const char* const* aFormatNames = sFormatNames,
                             int32_t            aWidth = 0,
                             int32_t            aHeight = 0,
                             const nsAString&   aStatus = EmptyString());

  nsCOMPtr<nsIStringBundle>     mStringBundle;
  static const char* const      sFormatNames[4];
  
private:
  enum                          {eWithNoInfo, eWithFile, eWithDim, eWithDimAndFile};
  bool                          mDocumentElementInserted;   
};


class MediaDocumentStreamListener: public nsIStreamListener
{
protected:
  virtual ~MediaDocumentStreamListener();

public:
  explicit MediaDocumentStreamListener(MediaDocument* aDocument);
  void SetStreamListener(nsIStreamListener *aListener);

  NS_DECL_ISUPPORTS

  NS_DECL_NSIREQUESTOBSERVER

  NS_DECL_NSISTREAMLISTENER

  nsRefPtr<MediaDocument>      mDocument;
  nsCOMPtr<nsIStreamListener>  mNextStream;
};

} 
} 

#endif
