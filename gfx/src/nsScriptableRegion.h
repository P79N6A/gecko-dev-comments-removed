






































#include "nsIScriptableRegion.h"
#include "gfxCore.h"
#include "nsISupports.h"
#include "nsRegion.h"

class NS_GFX nsScriptableRegion : public nsIScriptableRegion {
public:
	nsScriptableRegion();

	NS_DECL_ISUPPORTS

	NS_DECL_NSISCRIPTABLEREGION

private:
	nsIntRegion mRegion;
};
