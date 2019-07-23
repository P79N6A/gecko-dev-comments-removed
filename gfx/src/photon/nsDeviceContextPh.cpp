




































#include <math.h>

#include "nspr.h"
#include "nsIPref.h"
#include "nsIServiceManager.h"
#include "nsCRT.h"
#include "nsReadableUtils.h"

#include "nsDeviceContextPh.h"
#include "nsRenderingContextPh.h"
#include "nsDeviceContextSpecPh.h"
#include "nsHashtable.h"

#include "nsPhGfxLog.h"

#define NS_TO_PH_RGB(ns) (ns & 0xff) << 16 | (ns & 0xff00) | ((ns >> 16) & 0xff)

nscoord nsDeviceContextPh::mDpi = 96;

static nsHashtable* mFontLoadCache = nsnull;

nsDeviceContextPh :: nsDeviceContextPh( )
  : DeviceContextImpl()
  {
  mTwipsToPixels = 1.0;
  mPixelsToTwips = 1.0;
  mDepth = 0 ;
  mSurface = NULL;
  mPixelScale = 1.0f;
  mWidthFloat = 0.0f;
  mHeightFloat = 0.0f;

  
  mWidth = -1;
  mHeight = -1;

  mSpec = nsnull;
  mDC = nsnull;
	mGC = nsnull;

	mIsPrintingStart = 0;
	}

nsDeviceContextPh :: ~nsDeviceContextPh( ) {
  nsDrawingSurfacePh *surf = (nsDrawingSurfacePh *)mSurface;

  NS_IF_RELEASE(surf);    
  mSurface = nsnull;

	if( mGC ) PgDestroyGC( mGC ); 

	if( mFontLoadCache ) { 
#ifdef DEBUG_Adrian
printf( "\n\n\n!!!!!!!!!!!!!!!!! ~nsDeviceContextPh is unloading the mFontLoadCache!!!!!!!!!!!!!!!!!!!\n\n" );
#endif
		delete mFontLoadCache;
		mFontLoadCache = nsnull;
		}
	NS_IF_RELEASE( mSpec );
	}

NS_IMETHODIMP nsDeviceContextPh :: Init( nsNativeWidget aWidget ) {
    
  CommonInit(NULL);
 
  
  return DeviceContextImpl::Init( aWidget );
	}



nsresult nsDeviceContextPh :: Init( nsNativeDeviceContext aContext, nsIDeviceContext *aOrigContext ) {
  float origscale, newscale, t2d, a2d;
    
  mDC = aContext;

  CommonInit(mDC);

  newscale = TwipsToDevUnits();
  origscale = aOrigContext->AppUnitsToDevUnits();
  
  mPixelScale = newscale / origscale;

  t2d = aOrigContext->TwipsToDevUnits();
  a2d = aOrigContext->AppUnitsToDevUnits();

  mAppUnitsToDevUnits = (a2d / t2d) * mTwipsToPixels;
  mDevUnitsToAppUnits = 1.0f / mAppUnitsToDevUnits;

	int w, h;
	GetPrinterRect( &w, &h );
	mWidthFloat = w;
	mHeightFloat = h;

	
	DeviceContextImpl::CommonInit( );

	return NS_OK;
	}

void nsDeviceContextPh :: GetPrinterRect( int *width, int *height ) {
	PhDim_t dim;
	const PhDim_t *psize;
	const PhRect_t 	*non_print;
	PhRect_t	rect, margins;
	const char *orientation = 0;
	int			tmp;
	PpPrintContext_t *pc = ((nsDeviceContextSpecPh *)mSpec)->GetPrintContext();
	
	memset( &rect, 0, sizeof(rect));
	memset( &margins, 0, sizeof(margins));

	PpPrintGetPC(pc, Pp_PC_PAPER_SIZE, (const void **)&psize );
	PpPrintGetPC(pc, Pp_PC_NONPRINT_MARGINS, (const void **)&non_print );
	dim.w = (psize->w - ( non_print->ul.x + non_print->lr.x )) * 100 / 1000;
	dim.h = (psize->h - ( non_print->ul.x + non_print->lr.x )) * 100 / 1000;

	PpPrintGetPC(pc, Pp_PC_ORIENTATION, (const void **)&orientation );

	if( *orientation ) {
		tmp = dim.w;
		dim.w = dim.h;
		dim.h = tmp;
		}

	
	PpPrintSetPC(pc, INITIAL_PC, 0 , Pp_PC_MARGINS, &rect ); 
	PpPrintSetPC(pc, INITIAL_PC, 0 , Pp_PC_SOURCE_SIZE, &dim );

	*width = dim.w;
	*height = dim.h;
	}


void nsDeviceContextPh :: CommonInit( nsNativeDeviceContext aDC ) {
  PRInt32           aWidth, aHeight;
  static int        initialized = 0;


	if( !mScreenManager ) mScreenManager = do_GetService("@mozilla.org/gfx/screenmanager;1");

  if( !initialized ) {
    initialized = 1;
    
    
    
    
    
    PRInt32 prefVal = -1;
    nsresult res;

    nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &res));
    if( NS_SUCCEEDED( res ) && prefs ) {
      res = prefs->GetIntPref("layout.css.dpi", &prefVal);
      if( NS_FAILED( res ) ) {
        prefVal = 96;
      	}

      prefs->RegisterCallback( "layout.css.dpi", prefChanged, (void *)this );
      if( prefVal >0 ) mDpi = prefVal;
    	}
  	}

  SetDPI( mDpi );

	GetDisplayInfo(aWidth, aHeight, mDepth);

	
	mWidthFloat  = (float) aWidth;
	mHeightFloat = (float) aHeight;
	}

NS_IMETHODIMP nsDeviceContextPh :: CreateRenderingContext( nsIRenderingContext *&aContext ) {

#ifdef NS_PRINT_PREVIEW
	
	if(mAltDC && ((mUseAltDC & kUseAltDCFor_CREATERC_PAINT) || (mUseAltDC & kUseAltDCFor_CREATERC_REFLOW))) {
		return mAltDC->CreateRenderingContext(aContext);
		}
#endif

  nsIRenderingContext *pContext;
  nsresult             rv;
  nsDrawingSurfacePh  *surf;
   
	pContext = new nsRenderingContextPh();
	
	if( nsnull != pContext ) {
	  NS_ADDREF(pContext);

	  surf = new nsDrawingSurfacePh();
	  if( nsnull != surf ) {
			PpPrintContext_t *pc = ((nsDeviceContextSpecPh *)mSpec)->GetPrintContext();
			mGC = PgCreateGC( 0 );
			rv = surf->Init( (PhDrawContext_t*)pc, mGC );
			if( NS_OK == rv ) rv = pContext->Init(this, surf);
			else rv = NS_ERROR_OUT_OF_MEMORY;
			}
		}
	else rv = NS_ERROR_OUT_OF_MEMORY;

	if( NS_OK != rv ) NS_IF_RELEASE( pContext );

	aContext = pContext;
	return rv;
	}


static char *FaceMessageFont, *FaceMenuFont, *FaceBalloonFont, *FaceGeneralFont;
static int SizeMessageFont, SizeMenuFont, SizeBalloonFont, SizeGeneralFont;
static short StyleMessageFont, StyleMenuFont, StyleBalloonFont, StyleGeneralFont;
static short WeightMessageFont, WeightMenuFont, WeightBalloonFont, WeightGeneralFont;
static int InitSystemFonts;


int nsDeviceContextPh :: ReadSystemFonts( ) const
{
	FILE *fp;
	char buffer[512];

	fp = fopen( "/usr/share/mozilla/system_fonts", "r" );
	if( !fp ) return -1;

	while ( fgets( buffer, 512, fp ) != NULL ) {
		int len, code;
		char *p, **face;
		int *size;
		short *style, *weight;

		
		if( buffer[0] == '#' || buffer[0] == ' ' || buffer[0] == '\n' ) continue;
		len = strlen( buffer );
		if( len<3 ) continue;

		if( buffer[len-1] == '\n' ) buffer[ len-1 ] = 0;

		code = buffer[0] << 8 | buffer[1];
		p = &buffer[2];
		switch( code ) {
			case 'h=':
				if( !strcmp( p, "General" ) ) {
					style = &StyleGeneralFont;
					face = &FaceGeneralFont;
					weight = &WeightGeneralFont;
					size = &SizeGeneralFont;
					}
				else if( !strcmp( p, "Message" ) ) {
					style = &StyleMessageFont;
					face = &FaceMessageFont;
					weight = &WeightMessageFont;
					size = &SizeMessageFont;
					}
				else if( !strcmp( p, "Menu" ) ) {
					style = &StyleMenuFont;
					face = &FaceMenuFont;
					weight = &WeightMenuFont;
					size = &SizeMenuFont;
					}
				else if( !strcmp( p, "Balloon" ) ) {
					style = &StyleBalloonFont;
					face = &FaceBalloonFont;
					weight = &WeightBalloonFont;
					size = &SizeBalloonFont;
					}
				break;
			case 'f=':
				*face = strdup( p );
				break;
			case 's=':
				*size = atoi( p );
				break;
			case 'w=':
				*weight = 0;
				if( strstr( p, "bold" ) )
					*weight = NS_FONT_WEIGHT_BOLD;
				else
					*weight = NS_FONT_WEIGHT_NORMAL;
				break;
			case 'S=':
				*style = 0;
				if( strstr( p, "italic" ) )
					*style = NS_FONT_STYLE_ITALIC;
				else
					*style = NS_FONT_STYLE_NORMAL;
				if( strstr( p, "antialias" ) )
					*style |= NS_FONT_STYLE_ANTIALIAS;

				break;
			}
		}

	fclose( fp );

	return 0;
}

void nsDeviceContextPh :: DefaultSystemFonts( ) const
{
	FaceMessageFont = "MessageFont";
	SizeMessageFont = 8;
	StyleMessageFont = NS_FONT_STYLE_NORMAL;
	WeightMessageFont = NS_FONT_WEIGHT_NORMAL;

	FaceMenuFont = "MenuFont";
	SizeMenuFont = 8;
	StyleMenuFont = NS_FONT_STYLE_NORMAL;
	WeightMenuFont = NS_FONT_WEIGHT_NORMAL;

	FaceBalloonFont = "BalloonFont";
	SizeBalloonFont = 8;
	StyleBalloonFont = NS_FONT_STYLE_NORMAL;
	WeightBalloonFont = NS_FONT_WEIGHT_NORMAL;

	FaceGeneralFont = "TextFont";
	SizeGeneralFont = 8;
	StyleGeneralFont = NS_FONT_STYLE_NORMAL;
	WeightGeneralFont = NS_FONT_WEIGHT_NORMAL;
}

NS_IMETHODIMP nsDeviceContextPh :: GetSystemFont( nsSystemFontID aID, nsFont *aFont) const
{

	if( !InitSystemFonts ) {
		InitSystemFonts = 1;
		DefaultSystemFonts( );
		ReadSystemFonts( );
		}

	aFont->decorations = NS_FONT_DECORATION_NONE;

  switch (aID) {
    case eSystemFont_Caption:      
		case eSystemFont_Icon:
		case eSystemFont_SmallCaption:
		case eSystemFont_StatusBar:
		case eSystemFont_Window:       
		case eSystemFont_Document:
		case eSystemFont_Workspace:
		case eSystemFont_Desktop:
		case eSystemFont_Info:
		case eSystemFont_Dialog:
		case eSystemFont_Button:
		case eSystemFont_PullDownMenu:
		case eSystemFont_List:
		case eSystemFont_Field:
		case eSystemFont_Widget:
	  	aFont->name.AssignWithConversion( FaceGeneralFont );
			aFont->style = StyleGeneralFont;
			aFont->weight = WeightGeneralFont;
			aFont->size = SizeGeneralFont / ( mAppUnitsToDevUnits * 0.68 ); 
			break;
		case eSystemFont_MessageBox:
			aFont->name.AssignWithConversion( FaceMessageFont );
			aFont->style = StyleMessageFont;
			aFont->weight = WeightMessageFont;
			aFont->size = SizeMessageFont / ( mAppUnitsToDevUnits * 0.68 ); 
			break;
		case eSystemFont_Tooltips:     
			aFont->name.AssignWithConversion( FaceBalloonFont );
			aFont->style = StyleBalloonFont;
			aFont->weight = WeightBalloonFont;
			aFont->size = SizeBalloonFont / ( mAppUnitsToDevUnits * 0.68 ); 
			break;
		case eSystemFont_Menu:
			aFont->name.AssignWithConversion( FaceMenuFont );
			aFont->style = StyleMenuFont;
			aFont->weight = WeightMenuFont;
			aFont->size = SizeMenuFont / ( mAppUnitsToDevUnits * 0.68 ); 
			break;
  	}

  aFont->systemFont = PR_TRUE;

  return NS_OK;
}


NS_IMETHODIMP nsDeviceContextPh :: CheckFontExistence( const nsString& aFontName ) {
  char *fontName = ToNewCString(aFontName);

  if( fontName ) {

#ifdef DEBUG_Adrian
printf( "\tCheckFontExistence for fontName=%s\n", fontName );
#endif

		nsCStringKey key( fontName );
		if( !mFontLoadCache ) mFontLoadCache = new nsHashtable();
		else {
			int value = ( int ) mFontLoadCache->Get( &key );
			if( value == 1 ) { 
				delete [] fontName;
#ifdef DEBUG_Adrian
printf( "\t\tFound it in cache it exists\n" );
#endif
				return NS_OK;
				}
			else if( value == 2 ) { 
				delete [] fontName;
#ifdef DEBUG_Adrian
printf( "\t\tFound it in cache it doesn't exist\n" );
#endif
				return NS_ERROR_FAILURE;
				}
			
#ifdef DEBUG_Adrian
printf( "\t\t Not Found in cache\n" );
#endif
			}

		
		
  	nsresult res;
		if( PfFindFont( (char *)fontName, 0, 0 ) ) {
			mFontLoadCache->Put( &key, (void*)1 );
			res = NS_OK;
			}
		else {
			mFontLoadCache->Put( &key, (void*)2 );
			res = NS_ERROR_FAILURE;
			}
		delete [] fontName;
		return res;
		}

	return NS_ERROR_FAILURE;
	}


NS_IMETHODIMP nsDeviceContextPh :: GetDeviceSurfaceDimensions( PRInt32 &aWidth, PRInt32 &aHeight ) {

#ifdef NS_PRINT_PREVIEW
	
	if (mAltDC && (mUseAltDC & kUseAltDCFor_SURFACE_DIM)) {
		return mAltDC->GetDeviceSurfaceDimensions(aWidth, aHeight);
		}
#endif

  if( mWidth == -1 ) mWidth = NSToIntRound(mWidthFloat * mDevUnitsToAppUnits);
  if( mHeight == -1 ) mHeight = NSToIntRound(mHeightFloat * mDevUnitsToAppUnits);

  aWidth = mWidth;
  aHeight = mHeight;

  return NS_OK;
	}

NS_IMETHODIMP nsDeviceContextPh::GetRect( nsRect &aRect ) {
	if( mScreenManager ) {
    nsCOMPtr<nsIScreen> screen;
    mScreenManager->GetPrimaryScreen( getter_AddRefs( screen ) );
    screen->GetRect(&aRect.x, &aRect.y, &aRect.width, &aRect.height);
    aRect.x = NSToIntRound(mDevUnitsToAppUnits * aRect.x);
    aRect.y = NSToIntRound(mDevUnitsToAppUnits * aRect.y);
    aRect.width = NSToIntRound(mDevUnitsToAppUnits * aRect.width);
    aRect.height = NSToIntRound(mDevUnitsToAppUnits * aRect.height);
		}
	else {
  	PRInt32 width, height;
  	GetDeviceSurfaceDimensions( width, height );
  	aRect.x = 0;
  	aRect.y = 0;
  	aRect.width = width;
  	aRect.height = height;
		}
  return NS_OK;
	}

NS_IMETHODIMP nsDeviceContextPh :: GetDeviceContextFor( nsIDeviceContextSpec *aDevice, nsIDeviceContext *&aContext ) {
	nsDeviceContextPh* devConPh = new nsDeviceContextPh(); 
	if (devConPh != nsnull) {
  		
    	nsresult rv = devConPh->QueryInterface(NS_GET_IID(nsIDeviceContext), (void**)&aContext);
	  	NS_ASSERTION(NS_SUCCEEDED(rv), "This has to support nsIDeviceContext");
	} else {
	    return NS_ERROR_OUT_OF_MEMORY;
	}

	devConPh->mSpec = aDevice;
	NS_ADDREF(aDevice);
	return devConPh->Init(NULL, this);
	}

nsresult nsDeviceContextPh::SetDPI( PRInt32 aDpi ) {
  const int pt2t = 82;

  mDpi = aDpi;

  
  mPixelsToTwips = float(NSToIntRound(float(NSIntPointsToTwips(pt2t)) / float(aDpi)));
  mTwipsToPixels = 1.0f / mPixelsToTwips;
  
  return NS_OK;
	}

int nsDeviceContextPh::prefChanged( const char *aPref, void *aClosure ) {
  nsDeviceContextPh *context = (nsDeviceContextPh*)aClosure;
  nsresult rv;

  if( nsCRT::strcmp(aPref, "layout.css.dpi")==0 )  {
    PRInt32 dpi;
    nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
    rv = prefs->GetIntPref(aPref, &dpi);
    if( NS_SUCCEEDED( rv ) ) context->SetDPI( dpi ); 
		}
  return 0;
	}

NS_IMETHODIMP nsDeviceContextPh :: BeginDocument(PRUnichar *t, PRUnichar* aPrintToFileName, PRInt32 aStartPage, PRInt32 aEndPage) {
	if( mSpec ) {
		PpPrintContext_t *pc = ((nsDeviceContextSpecPh *)mSpec)->GetPrintContext();
		PpStartJob(pc);
		mIsPrintingStart = 1;
		}
	return NS_OK;
	}

NS_IMETHODIMP nsDeviceContextPh :: EndDocument( void ) {
	if( mSpec ) {
  	PpPrintContext_t *pc = ((nsDeviceContextSpecPh *)mSpec)->GetPrintContext();
  	PpEndJob(pc);
		}
  return NS_OK;
	}

NS_IMETHODIMP nsDeviceContextPh :: AbortDocument( void ) {
  return EndDocument();
	}

NS_IMETHODIMP nsDeviceContextPh :: BeginPage( void ) {
	if( mSpec ) {
		PpPrintContext_t *pc = ((nsDeviceContextSpecPh *)mSpec)->GetPrintContext();
		PpContinueJob( pc );
		if( !mIsPrintingStart ) PpPrintNewPage( pc );
		mIsPrintingStart = 0;
		}
	return NS_OK;
	}

NS_IMETHODIMP nsDeviceContextPh :: EndPage( void ) {
	if( mSpec ) {
		PpPrintContext_t *pc = ((nsDeviceContextSpecPh *)mSpec)->GetPrintContext();
		PpSuspendJob(pc);
		}
	return NS_OK;
	}




nsresult nsDeviceContextPh :: GetDisplayInfo( PRInt32 &aWidth, PRInt32 &aHeight, PRUint32 &aDepth ) {
  nsresult    			res = NS_ERROR_FAILURE;
  PhSysInfo_t       SysInfo;
  PhRect_t          rect;
  char              *p = NULL;
  int               inp_grp;
  PhRid_t           rid;

  
  aWidth  = 0;
  aHeight = 0;
  aDepth  = 0;
  
	
	p = getenv("PHIG");
	if( p ) inp_grp = atoi( p );
	else inp_grp = 1;

	PhQueryRids( 0, 0, inp_grp, Ph_GRAFX_REGION, 0, 0, 0, &rid, 1 );
	PhWindowQueryVisible( Ph_QUERY_IG_POINTER, 0, inp_grp, &rect );
	aWidth  = rect.lr.x - rect.ul.x + 1;
	aHeight = rect.lr.y - rect.ul.y + 1;  

	
	if( PhQuerySystemInfo( rid, NULL, &SysInfo ) ) {
		
		if( SysInfo.gfx.valid_fields & Ph_GFX_COLOR_BITS ) {
			aDepth = SysInfo.gfx.color_bits;
			res = NS_OK;
			}
		}
  return res;
	}
