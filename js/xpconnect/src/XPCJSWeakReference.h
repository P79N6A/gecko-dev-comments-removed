





#ifndef xpcjsweakreference_h___
#define xpcjsweakreference_h___

#include "xpcIJSWeakReference.h"
#include "nsIWeakReference.h"
#include "mozilla/Attributes.h"

class xpcJSWeakReference MOZ_FINAL : public xpcIJSWeakReference
{
    ~xpcJSWeakReference() {}

public:
    xpcJSWeakReference();
    nsresult Init(JSContext* cx, const JS::Value& object);

    NS_DECL_ISUPPORTS
    NS_DECL_XPCIJSWEAKREFERENCE

private:
    nsCOMPtr<nsIWeakReference> mReferent;
};

#endif 
