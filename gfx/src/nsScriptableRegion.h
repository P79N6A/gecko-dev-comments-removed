





#include "nsIScriptableRegion.h"
#include "gfxCore.h"
#include "nsISupports.h"
#include "nsRegion.h"
#include "mozilla/Attributes.h"

class NS_GFX nsScriptableRegion MOZ_FINAL : public nsIScriptableRegion {
public:
	nsScriptableRegion();

	NS_DECL_ISUPPORTS

	NS_DECL_NSISCRIPTABLEREGION

private:
	nsIntRegion mRegion;
};
