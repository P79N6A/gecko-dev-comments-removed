






































#ifdef MOZ_LOGGING
#define FORCE_PR_LOG 1 /* Allow logging in the release build */
#endif 
#include "prlog.h"
 
#include "nsDeviceContextXP.h"
#include "nsRenderingContextXp.h"
#include "nsFontMetricsXlib.h"
#include "nsIDeviceContext.h"
#include "nsIDeviceContextSpecXPrint.h"
#include "nspr.h"
#include "nsXPrintContext.h"

#ifdef PR_LOGGING
static PRLogModuleInfo *nsDeviceContextXpLM = PR_NewLogModule("nsDeviceContextXp");
#endif 




nsDeviceContextXp :: nsDeviceContextXp()
  : nsDeviceContextX(),
    mPrintContext(nsnull),
    mSpec(nsnull),
    mParentDeviceContext(nsnull),
    mFontMetricsContext(nsnull),
    mRCContext(nsnull)
{
}





nsDeviceContextXp :: ~nsDeviceContextXp() 
{ 
  DestroyXPContext();
}


NS_IMETHODIMP
nsDeviceContextXp::SetSpec(nsIDeviceContextSpec* aSpec)
{
  nsresult  rv = NS_ERROR_FAILURE;
  PR_LOG(nsDeviceContextXpLM, PR_LOG_DEBUG, ("nsDeviceContextXp::SetSpec()\n"));

  nsCOMPtr<nsIDeviceContextSpecXp> xpSpec;

  mSpec = aSpec;

  if (mPrintContext)
    DestroyXPContext(); 
    
  mPrintContext = new nsXPrintContext();
  if (!mPrintContext)
    return  NS_ERROR_OUT_OF_MEMORY;
    
  xpSpec = do_QueryInterface(mSpec, &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = mPrintContext->Init(this, xpSpec);

    if (NS_FAILED(rv)) {
      DestroyXPContext();
    }
  }
 
  return rv;
}

NS_IMPL_ISUPPORTS_INHERITED1(nsDeviceContextXp,
                             DeviceContextImpl,
                             nsIDeviceContextXp)






NS_IMETHODIMP
nsDeviceContextXp::InitDeviceContextXP(nsIDeviceContext *aCreatingDeviceContext,nsIDeviceContext *aParentContext)
{
  nsresult rv;

  
  float origscale, newscale;
  float t2d, a2d;
  int   print_x_resolution,
        print_y_resolution;

  mPrintContext->GetPrintResolution(print_x_resolution, print_y_resolution);
  
  if (print_x_resolution != print_y_resolution) {
    PR_LOG(nsDeviceContextXpLM, PR_LOG_DEBUG, ("print_x_resolution != print_y_resolution not yet supported by Mozilla's layout engine\n"));
    return NS_ERROR_NOT_IMPLEMENTED; 
  }

  mPixelsToTwips = (float)NSIntPointsToTwips(72) / (float)print_x_resolution;
  mTwipsToPixels = 1.0f / mPixelsToTwips;

  newscale = TwipsToDevUnits();
  origscale = aParentContext->TwipsToDevUnits();

  mCPixelScale = newscale / origscale;

  t2d = aParentContext->TwipsToDevUnits();
  a2d = aParentContext->AppUnitsToDevUnits();

  mAppUnitsToDevUnits = (a2d / t2d) * mTwipsToPixels;
  mDevUnitsToAppUnits = 1.0f / mAppUnitsToDevUnits;

  NS_ASSERTION(aParentContext, "aCreatingDeviceContext cannot be NULL!!!");
  mParentDeviceContext = aParentContext;

  
  DeleteRenderingContextXlibContext(mRCContext);
  DeleteFontMetricsXlibContext(mFontMetricsContext);
  mRCContext          = nsnull;
  mFontMetricsContext = nsnull;
 
  rv = CreateFontMetricsXlibContext(this, PR_TRUE, &mFontMetricsContext);
  if (NS_FAILED(rv))
    return rv;

  rv = CreateRenderingContextXlibContext(this, &mRCContext);
  if (NS_FAILED(rv))
    return rv;
   
  return NS_OK;
}



NS_IMETHODIMP nsDeviceContextXp :: CreateRenderingContext(nsIRenderingContext *&aContext)
{
  nsresult rv;
   
  aContext = nsnull;

  NS_ENSURE_TRUE(mPrintContext != nsnull, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsRenderingContextXp> renderingContext = new nsRenderingContextXp();
  if (!renderingContext)
    return NS_ERROR_OUT_OF_MEMORY;
     
  rv = renderingContext->Init(this);

  if (NS_SUCCEEDED(rv)) {
    aContext = renderingContext;
    NS_ADDREF(aContext);
  }

  return rv;
}

NS_IMETHODIMP nsDeviceContextXp::CreateRenderingContextInstance(nsIRenderingContext *&aContext)
{
  nsCOMPtr<nsIRenderingContext> renderingContext = new nsRenderingContextXp();
  if (!renderingContext)
    return NS_ERROR_OUT_OF_MEMORY;
         
  aContext = renderingContext;
  NS_ADDREF(aContext);
  
  return NS_OK;
}




NS_IMETHODIMP nsDeviceContextXp :: SupportsNativeWidgets(PRBool &aSupportsWidgets)
{
  aSupportsWidgets = PR_FALSE;
  return NS_OK;
}





NS_IMETHODIMP nsDeviceContextXp :: CheckFontExistence(const nsString& aFontName)
{
  return nsFontMetricsXlib::FamilyExists(mFontMetricsContext, aFontName);
}

NS_IMETHODIMP nsDeviceContextXp :: GetSystemFont(nsSystemFontID aID, 
                                                 nsFont *aFont) const
{
  if (mParentDeviceContext != nsnull) {
    return mParentDeviceContext->GetSystemFont(aID, aFont);
  }
  return NS_ERROR_FAILURE;
}




NS_IMETHODIMP nsDeviceContextXp::GetDeviceSurfaceDimensions(PRInt32 &aWidth, 
                                                        PRInt32 &aHeight)
{
  NS_ENSURE_TRUE(mPrintContext != nsnull, NS_ERROR_NULL_POINTER);

  float width, height;
  width  = (float) mPrintContext->GetWidth();
  height = (float) mPrintContext->GetHeight();

  aWidth  = NSToIntRound(width  * mDevUnitsToAppUnits);
  aHeight = NSToIntRound(height * mDevUnitsToAppUnits);

  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextXp::GetRect(nsRect &aRect)
{
  NS_ENSURE_TRUE(mPrintContext != nsnull, NS_ERROR_NULL_POINTER);

  PRInt32 width, height;
  nsresult rv;
  rv = GetDeviceSurfaceDimensions(width, height);
  aRect.x = 0;
  aRect.y = 0;
  aRect.width = width;
  aRect.height = height;
  return rv;
}




NS_IMETHODIMP nsDeviceContextXp::GetClientRect(nsRect &aRect)
{
  return GetRect(aRect);
}




NS_IMETHODIMP nsDeviceContextXp::GetDeviceContextFor(nsIDeviceContextSpec *aDevice, nsIDeviceContext *&aContext)
{
  aContext = nsnull;
  return NS_OK;
}




NS_IMETHODIMP nsDeviceContextXp::BeginDocument(PRUnichar * aTitle, PRUnichar* aPrintToFileName, PRInt32 aStartPage, PRInt32 aEndPage)
{  
  PR_LOG(nsDeviceContextXpLM, PR_LOG_DEBUG, ("nsDeviceContextXp::BeginDocument()\n"));

  NS_ENSURE_TRUE(mPrintContext != nsnull, NS_ERROR_NULL_POINTER);

  return mPrintContext->BeginDocument(aTitle, aPrintToFileName, aStartPage, aEndPage);
}


void nsDeviceContextXp::DestroyXPContext()
{
  PR_LOG(nsDeviceContextXpLM, PR_LOG_DEBUG, ("nsDeviceContextXp::DestroyXPContext()\n"));

  if (!mPrintContext)
    return;

  




  FlushFontCache();
  DeleteRenderingContextXlibContext(mRCContext);
  DeleteFontMetricsXlibContext(mFontMetricsContext);
  mRCContext          = nsnull;
  mFontMetricsContext = nsnull;

  mPrintContext = nsnull; 
}




NS_IMETHODIMP nsDeviceContextXp::EndDocument(void)
{
  PR_LOG(nsDeviceContextXpLM, PR_LOG_DEBUG, ("nsDeviceContextXp::EndDocument()\n"));

  NS_ENSURE_TRUE(mPrintContext != nsnull, NS_ERROR_NULL_POINTER);

  nsresult rv = mPrintContext->EndDocument();
  DestroyXPContext();
  return rv;
}




NS_IMETHODIMP nsDeviceContextXp::AbortDocument(void)
{
  PR_LOG(nsDeviceContextXpLM, PR_LOG_DEBUG, ("nsDeviceContextXp::AbortDocument()\n"));

  NS_ENSURE_TRUE(mPrintContext != nsnull, NS_ERROR_NULL_POINTER);

  nsresult rv = mPrintContext->AbortDocument();
  DestroyXPContext();
  return rv;
}




NS_IMETHODIMP nsDeviceContextXp::BeginPage(void)
{
  NS_ENSURE_TRUE(mPrintContext != nsnull, NS_ERROR_NULL_POINTER);

  return mPrintContext->BeginPage();
}





NS_IMETHODIMP nsDeviceContextXp::EndPage(void)
{
  NS_ENSURE_TRUE(mPrintContext != nsnull, NS_ERROR_NULL_POINTER);

  return mPrintContext->EndPage();
}

NS_IMETHODIMP
nsDeviceContextXp::GetPrintContext(nsXPrintContext*& aContext)
{
  aContext = mPrintContext;
  return NS_OK;
}

class nsFontCacheXp : public nsFontCache
{
public:
  
  virtual nsresult CreateFontMetricsInstance(nsIFontMetrics** aResult);
};


nsresult nsFontCacheXp::CreateFontMetricsInstance(nsIFontMetrics** aResult)
{
  NS_PRECONDITION(aResult, "null out param");
  nsIFontMetrics *fm = new nsFontMetricsXlib();
  if (!fm)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(fm);
  *aResult = fm;
  return NS_OK;
}


NS_IMETHODIMP nsDeviceContextXp::CreateFontCache()
{
  mFontCache = new nsFontCacheXp();
  if (nsnull == mFontCache) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return mFontCache->Init(this);
}


