




































#ifndef nsContentDLF_h__
#define nsContentDLF_h__

#include "nsIDocumentLoaderFactory.h"
#include "nsIDocumentViewer.h"
#include "nsIDocument.h"

class nsICSSStyleSheet;
class nsIChannel;
class nsIComponentManager;
class nsIContentViewer;
class nsIDocumentViewer;
class nsIFile;
class nsIInputStream;
class nsILoadGroup;
class nsIStreamListener;
struct nsModuleComponentInfo;

class nsContentDLF : public nsIDocumentLoaderFactory
{
public:
  nsContentDLF();
  virtual ~nsContentDLF();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOCUMENTLOADERFACTORY

  nsresult InitUAStyleSheet();

  nsresult CreateDocument(const char* aCommand,
                          nsIChannel* aChannel,
                          nsILoadGroup* aLoadGroup,
                          nsISupports* aContainer,
                          const nsCID& aDocumentCID,
                          nsIStreamListener** aDocListener,
                          nsIContentViewer** aDocViewer);

  nsresult CreateXULDocument(const char* aCommand,
                             nsIChannel* aChannel,
                             nsILoadGroup* aLoadGroup,
                             const char* aContentType,
                             nsISupports* aContainer,
                             nsISupports* aExtraInfo,
                             nsIStreamListener** aDocListener,
                             nsIContentViewer** aDocViewer);

  static nsICSSStyleSheet* gUAStyleSheet;

#ifdef MOZ_SVG
  static NS_IMETHODIMP RegisterSVG();
  static NS_IMETHODIMP UnregisterSVG();
#endif

  static NS_IMETHODIMP
  RegisterDocumentFactories(nsIComponentManager* aCompMgr,
                            nsIFile* aPath,
                            const char *aLocation,
                            const char *aType,
                            const nsModuleComponentInfo* aInfo);

  static NS_IMETHODIMP
  UnregisterDocumentFactories(nsIComponentManager* aCompMgr,
                              nsIFile* aPath,
                              const char* aRegistryLocation,
                              const nsModuleComponentInfo* aInfo);

private:
  static nsresult EnsureUAStyleSheet();
};

nsresult
NS_NewContentDocumentLoaderFactory(nsIDocumentLoaderFactory** aResult);

#endif

