





#ifndef nsScriptableRegion_h
#define nsScriptableRegion_h

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
        ~nsScriptableRegion() {}
	nsIntRegion mRegion;
};

#endif
