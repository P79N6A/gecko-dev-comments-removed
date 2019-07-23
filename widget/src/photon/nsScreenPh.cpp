




































#include "nsScreenPh.h"

#include <Pt.h>
#include "nsPhGfxLog.h"

nsScreenPh :: nsScreenPh ( ) {
  PhSysInfo_t       SysInfo;
  PhRect_t          rect;
  char              *p;
  int               inp_grp;
  PhRid_t           rid;
  PhRegion_t        region;
  
	p = NULL;

	
	
	p = getenv("PHIG");
	if( p ) inp_grp = atoi(p);
	else inp_grp = 1;

	PhQueryRids( 0, 0, inp_grp, Ph_INPUTGROUP_REGION, 0, 0, 0, &rid, 1 );
	PhRegionQuery( rid, &region, &rect, NULL, 0 );
	inp_grp = region.input_group;
	PhWindowQueryVisible( Ph_QUERY_IG_POINTER, 0, inp_grp, &rect );
	mWidth  = rect.lr.x - rect.ul.x + 1;
	mHeight = rect.lr.y - rect.ul.y + 1;  

	
	if( PhQuerySystemInfo(rid, NULL, &SysInfo ) ) {
		
		if( SysInfo.gfx.valid_fields & Ph_GFX_COLOR_BITS ) mPixelDepth = SysInfo.gfx.color_bits;
		}
	}

nsScreenPh :: ~nsScreenPh( ) { }


NS_IMPL_ISUPPORTS1(nsScreenPh, nsIScreen)

NS_IMETHODIMP nsScreenPh :: GetPixelDepth( PRInt32 *aPixelDepth ) {
	*aPixelDepth = mPixelDepth;
	return NS_OK;
	} 


NS_IMETHODIMP nsScreenPh :: GetColorDepth( PRInt32 *aColorDepth ) {
  return GetPixelDepth ( aColorDepth );
	}

NS_IMETHODIMP nsScreenPh :: GetRect( PRInt32 *outLeft, PRInt32 *outTop, PRInt32 *outWidth, PRInt32 *outHeight ) {
  *outTop = 0;
  *outLeft = 0;
  *outWidth = mWidth;
  *outHeight = mHeight;
  return NS_OK;
	}

NS_IMETHODIMP nsScreenPh :: GetAvailRect( PRInt32 *outLeft, PRInt32 *outTop, PRInt32 *outWidth, PRInt32 *outHeight ) {
  *outTop = 0;
  *outLeft = 0;
  *outWidth = mWidth;
  *outHeight = mHeight;
  return NS_OK;
	}
