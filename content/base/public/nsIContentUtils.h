



































#ifndef nsIContentUtils_h__
#define nsIContentUtils_h__

#include "nsIDocumentLoaderFactory.h"
#include "nsCOMPtr.h"
#include "nsAString.h"
#include "nsMargin.h"


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

#endif 
