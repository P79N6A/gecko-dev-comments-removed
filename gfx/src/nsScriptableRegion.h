






































#include "nsIScriptableRegion.h"
#include "gfxCore.h"

class nsIRegion;




class NS_GFX nsScriptableRegion : public nsIScriptableRegion {
public:
	nsScriptableRegion(nsIRegion* region);
	virtual ~nsScriptableRegion();
	
	NS_DECL_ISUPPORTS

	NS_DECL_NSISCRIPTABLEREGION

private:
	nsIRegion* mRegion;
};
