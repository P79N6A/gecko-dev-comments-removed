




































#include "nsNativeAppSupport.h"

#define PX_IMAGE_MODULES
#define PX_BMP_SUPPORT
#include <photon/PxImage.h>

#include <Ph.h>
#include <Pt.h>

class nsSplashScreenPh : public nsISplashScreen {
public:
    nsSplashScreenPh()
        : mDialog( 0 ), mRefCnt( 0 ) {

     
      if( PtInit( NULL ) )
         exit( EXIT_FAILURE );

     
      PgSetDrawBufferSize( 0x8000 );
    }
    ~nsSplashScreenPh() {
       Hide();
    }

    NS_IMETHOD Show();
    NS_IMETHOD Hide();

    
    NS_IMETHOD_(nsrefcnt) AddRef() {
        mRefCnt++;
        return mRefCnt;
    }
    NS_IMETHOD_(nsrefcnt) Release() {
        --mRefCnt;
        if ( !mRefCnt ) {
            delete this;
            return 0;
        }
        return mRefCnt;
    }
    NS_IMETHOD QueryInterface( const nsIID &iid, void**p ) {
        nsresult rv = NS_OK;
        if ( p ) {
            *p = 0;
            if ( iid.Equals( NS_GET_IID( nsISplashScreen ) ) ) {
                nsISplashScreen *result = this;
                *p = result;
                NS_ADDREF( result );
            } else if ( iid.Equals( NS_GET_IID( nsISupports ) ) ) {
                nsISupports *result = NS_STATIC_CAST( nsISupports*, this );
                *p = result;
                NS_ADDREF( result );
            } else {
                rv = NS_NOINTERFACE;
            }
        } else {
            rv = NS_ERROR_NULL_POINTER;
        }
        return rv;
    }

    PtWidget_t *mDialog;
    nsrefcnt mRefCnt;
}; 


NS_IMETHODIMP
nsSplashScreenPh::Show()
{
  PhImage_t  *img = nsnull;
  char       *p = NULL;
  char       *splash = NULL;
  char       splash_path[256];
  int         inp_grp,n=0;
  PhRid_t     rid;
  PhRegion_t  region;
  PhRect_t    rect;
  PRInt32     aWidth, aHeight;
  PhRect_t  console;
  struct stat sbuf;

   
	 p = getenv("PHIG");
	 if( p ) inp_grp = atoi( p );
	 else inp_grp = 1;

   PhQueryRids( 0, 0, inp_grp, Ph_INPUTGROUP_REGION, 0, 0, 0, &rid, 1 );
   PhRegionQuery( rid, &region, &rect, NULL, 0 );
   inp_grp = region.input_group;
   PhWindowQueryVisible( Ph_QUERY_INPUT_GROUP | Ph_QUERY_EXACT, 0, inp_grp, &rect );
   aWidth  = rect.lr.x - rect.ul.x + 1;
   aHeight = rect.lr.y - rect.ul.y + 1;  

   
   if (PhWindowQueryVisible( Ph_QUERY_GRAPHICS, 0, inp_grp, &console ) == 0)
      
   mDialog = nsnull;   
	
   
   splash = getenv( "MOZILLA_FIVE_HOME" );
   if( splash )  {
			sprintf( splash_path, "%s/splash.bmp", splash );
      if( ( img = PxLoadImage( splash_path, NULL ) ) != NULL )
      {
       PtArg_t     arg[10];
       PhPoint_t   pos;

      img->flags = img->flags | Ph_RELEASE_IMAGE_ALL;

      pos.x = (aWidth/2)  - (img->size.w/2);
      pos.y = (aHeight/2) - (img->size.h/2);

      pos.x += console.ul.x;
      pos.y += console.ul.y;

      PtSetArg( &arg[n++], Pt_ARG_DIM, &img->size, 0 );
      PtSetArg( &arg[n++], Pt_ARG_POS, &pos, 0 );
      PtSetArg( &arg[n++], Pt_ARG_WINDOW_RENDER_FLAGS, 0, 0xFFFFFFFF );
      PtSetArg( &arg[n++], Pt_ARG_FILL_COLOR, Pg_BLACK, 0 );
      PtSetArg( &arg[n++], Pt_ARG_WINDOW_MANAGED_FLAGS, Ph_WM_CLOSE | Ph_WM_TOFRONT | Ph_WM_TOBACK | Ph_WM_MOVE | Ph_WM_RESIZE, ~0 );
      mDialog = PtCreateWidget( PtWindow, NULL, n, arg );

      n=0;
      pos.x = pos.y = 0;
      PtSetArg( &arg[n++], Pt_ARG_POS, &pos, 0 );
      PtSetArg( &arg[n++], Pt_ARG_LABEL_TYPE, Pt_IMAGE, 0 );
      PtSetArg( &arg[n++], Pt_ARG_LABEL_DATA, img, sizeof(PhImage_t) );
      PtSetArg( &arg[n++], Pt_ARG_MARGIN_HEIGHT, 0, 0);
      PtSetArg( &arg[n++], Pt_ARG_MARGIN_WIDTH, 0, 0);
      PtCreateWidget( PtLabel, mDialog, n, arg );
      PtRealizeWidget( mDialog );
      PtFlush();
      }
   }
#ifdef DEBUG
   else
   {
     fprintf( stderr, "Error loading splash screen image: %s\n", splash );
   }
#endif

   return NS_OK;
}

NS_IMETHODIMP
nsSplashScreenPh::Hide() {
	if ( mDialog ) {
        PtDestroyWidget(mDialog);
        mDialog = nsnull;
	}
    return NS_OK;
}

nsresult NS_CreateSplashScreen(nsISplashScreen**aResult)
{
  if ( aResult ) {	
      *aResult = new nsSplashScreenPh;
      if ( *aResult ) {
          NS_ADDREF( *aResult );
          return NS_OK;
      } else {
          return NS_ERROR_OUT_OF_MEMORY;
      }
  } else {
      return NS_ERROR_NULL_POINTER;
  }
}

PRBool NS_CanRun() 
{
	return PR_TRUE;
}
