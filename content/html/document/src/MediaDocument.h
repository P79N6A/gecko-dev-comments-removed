




































#ifndef mozilla_dom_MediaDocument_h
#define mozilla_dom_MediaDocument_h

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

  virtual nsresult Init();

  virtual nsresult StartDocumentLoad(const char*         aCommand,
                                     nsIChannel*         aChannel,
                                     nsILoadGroup*       aLoadGroup,
                                     nsISupports*        aContainer,
                                     nsIStreamListener** aDocListener,
                                     PRBool              aReset = PR_TRUE,
                                     nsIContentSink*     aSink = nsnull);

protected:
  virtual nsresult CreateSyntheticDocument();

  friend class MediaDocumentStreamListener;
  nsresult StartLayout();

  void GetFileName(nsAString& aResult);

  
  
  
  
  
  
  
  
  
  
  
  void UpdateTitleAndCharset(const nsACString&  aTypeStr,
                             const char* const* aFormatNames = sFormatNames,
                             PRInt32            aWidth = 0,
                             PRInt32            aHeight = 0,
                             const nsAString&   aStatus = EmptyString());

  nsCOMPtr<nsIStringBundle>     mStringBundle;
  static const char* const      sFormatNames[4];

private:
  enum                          {eWithNoInfo, eWithFile, eWithDim, eWithDimAndFile};
};


class MediaDocumentStreamListener: public nsIStreamListener
{
public:
  MediaDocumentStreamListener(MediaDocument *aDocument);
  virtual ~MediaDocumentStreamListener();
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
