




































#ifndef nsMediaDocument_h___
#define nsMediaDocument_h___

#include "nsHTMLDocument.h"
#include "nsGenericHTMLElement.h"
#include "nsAutoPtr.h"
#include "nsIStringBundle.h"

#define NSMEDIADOCUMENT_PROPERTIES_URI "chrome://global/locale/layout/MediaDocument.properties"

class nsMediaDocument : public nsHTMLDocument
{
public:
  nsMediaDocument();
  virtual ~nsMediaDocument();

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

  friend class nsMediaDocumentStreamListener;
  nsresult StartLayout();

  
  
  
  
  
  
  
  
  
  
  
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


class nsMediaDocumentStreamListener: public nsIStreamListener
{
public:
  nsMediaDocumentStreamListener(nsMediaDocument *aDocument);
  virtual ~nsMediaDocumentStreamListener();
  void SetStreamListener(nsIStreamListener *aListener);

  NS_DECL_ISUPPORTS

  NS_DECL_NSIREQUESTOBSERVER

  NS_DECL_NSISTREAMLISTENER

protected:
  nsRefPtr<nsMediaDocument>    mDocument;
  nsCOMPtr<nsIStreamListener>  mNextStream;
};


#endif
