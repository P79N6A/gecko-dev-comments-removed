



































#ifndef nsIContentUtils_h__
#define nsIContentUtils_h__

#include "nsIDocumentLoaderFactory.h"
#include "nsCOMPtr.h"
#include "nsAString.h"
#include "nsMargin.h"

class nsIInterfaceRequestor;


#define NS_ICONTENTUTILS_IID \
{ 0x3682dd99, 0x8560, 0x44f4, \
  { 0x9b, 0x8f, 0xcc, 0xce, 0x9d, 0x7b, 0x96, 0xfb } }

class nsIContentUtils : public nsISupports
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONTENTUTILS_IID)
    NS_DECL_ISUPPORTS

    virtual PRBool IsSafeToRunScript();
    virtual PRBool ParseIntMarginValue(const nsAString& aString, nsIntMargin& result);

    enum ContentViewerType
    {
        TYPE_UNSUPPORTED,
        TYPE_CONTENT,
        TYPE_PLUGIN,
        TYPE_UNKNOWN
    };

    virtual already_AddRefed<nsIDocumentLoaderFactory>
    FindInternalContentViewer(const char* aType,
                              ContentViewerType* aLoaderType = NULL);
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIContentUtils, NS_ICONTENTUTILS_IID)


#define NS_ICONTENTUTILS2_IID \
{ 0xc7193287, 0x3e3d, 0x467f, \
{ 0xb6, 0xda, 0x47, 0xb9, 0x14, 0xeb, 0x4c, 0x83 } }

class nsIContentUtils2 : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONTENTUTILS2_IID)
  NS_DECL_ISUPPORTS

  virtual nsIInterfaceRequestor* GetSameOriginChecker();
  
  virtual nsresult CheckSameOrigin(nsIChannel *aOldChannel, nsIChannel *aNewChannel);
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIContentUtils2, NS_ICONTENTUTILS2_IID)

#ifndef MOZ_ENABLE_LIBXUL



#define NS_ICONTENTUTILS_MOZILLA_2_0_BRANCH_IID \
{ 0x0fe8099c, 0x622a, 0x4c79, \
{ 0xb0, 0x02, 0x55, 0xf0, 0x44, 0x34, 0x00, 0x30 } }

class nsIContentUtils_MOZILLA_2_0_BRANCH : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONTENTURILS_MOZILLA_2_0_BRANCH_IID)
  NS_DECL_ISUPPORTS

  virtual nsresult DispatchTrustedEvent(nsIDocument* aDoc,
                                        nsISupports* aTarget,
                                        const nsAString& aEventName,
                                        PRBool aCanBubble,
                                        PRBool aCancelable,
                                        PRBool *aDefaultAction = nsnull);
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIContentUtils_MOZILLA_2_0_BRANCH, NS_ICONTENTUTILS_MOZILLA_2_0_BRANCH_IID)

#endif

#endif 
