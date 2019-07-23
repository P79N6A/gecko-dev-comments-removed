







































#include "gfx-config.h"
 






#define WE_DO_NOT_SUPPORT_MULTIPLE_PRINT_DEVICECONTEXTS 1

#define FORCE_PR_LOG
#define PR_LOGGING 1
#include "prlog.h"

#include "nsDeviceContextPS.h"
#include "nsRenderingContextPS.h"
#include "nsIServiceManager.h"
#include "nsIPref.h"
#include "nsString.h"
#include "nsFontMetricsPS.h"
#include "nsPostScriptObj.h"
#include "nspr.h"
#include "nsILanguageAtomService.h"
#include "nsPrintJobPS.h"
#include "nsPrintJobFactoryPS.h"
#if defined(MOZ_ENABLE_FREETYPE2) || defined(MOZ_ENABLE_XFT)
#include "nsType1.h"
#endif

#ifdef PR_LOGGING
static PRLogModuleInfo *nsDeviceContextPSLM = PR_NewLogModule("nsDeviceContextPS");
#endif 

nsIAtom* gUsersLocale = nsnull;

#ifdef WE_DO_NOT_SUPPORT_MULTIPLE_PRINT_DEVICECONTEXTS
static int instance_counter = 0;
#endif 

static PRBool
FreePSFontGeneratorList(nsHashKey *aKey, void *aData, void *aClosure)
{
  nsPSFontGenerator *psFG = (nsPSFontGenerator*)aData;
  if (psFG) {
    delete psFG;
    psFG = nsnull;
  }
  return PR_TRUE;
}





nsDeviceContextPS :: nsDeviceContextPS()
  : DeviceContextImpl(),
  mSpec(nsnull),
  mParentDeviceContext(nsnull),
  mPrintJob(nsnull),
  mPSObj(nsnull),
  mPSFontGeneratorList(nsnull)
{ 
  PR_LOG(nsDeviceContextPSLM, PR_LOG_DEBUG, ("nsDeviceContextPS::nsDeviceContextPS()\n"));

#ifdef WE_DO_NOT_SUPPORT_MULTIPLE_PRINT_DEVICECONTEXTS
  instance_counter++;
  NS_ASSERTION(instance_counter < 2, "Cannot have more than one print device context.");
#endif 
}





nsDeviceContextPS::~nsDeviceContextPS()
{
  PR_LOG(nsDeviceContextPSLM, PR_LOG_DEBUG, ("nsDeviceContextPS::~nsDeviceContextPS()\n"));

  delete mPSObj;
  delete mPrintJob;
  mParentDeviceContext = nsnull;

#ifdef WE_DO_NOT_SUPPORT_MULTIPLE_PRINT_DEVICECONTEXTS
  instance_counter--;
  NS_ASSERTION(instance_counter >= 0, "We cannot have less than zero instances.");
#endif 

  if (mPSFontGeneratorList) {
    mPSFontGeneratorList->Reset(FreePSFontGeneratorList, nsnull);
    delete mPSFontGeneratorList;
    mPSFontGeneratorList = nsnull;
  }
  NS_IF_RELEASE(gUsersLocale);
}

NS_IMETHODIMP
nsDeviceContextPS::SetSpec(nsIDeviceContextSpec* aSpec)
{
  PR_LOG(nsDeviceContextPSLM, PR_LOG_DEBUG, ("nsDeviceContextPS::SetSpec()\n"));
  NS_PRECONDITION(!mPSObj, "Already have a postscript object");
  NS_PRECONDITION(!mPrintJob, "Already have a printjob object");

#ifdef WE_DO_NOT_SUPPORT_MULTIPLE_PRINT_DEVICECONTEXTS
  NS_ASSERTION(instance_counter < 2, "Cannot have more than one print device context.");
  if (instance_counter > 1) {
    return NS_ERROR_GFX_PRINTER_PRINT_WHILE_PREVIEW;
  }
#endif 

  mSpec = aSpec;

  mPSObj = new nsPostScriptObj();
  if (!mPSObj)
    return  NS_ERROR_OUT_OF_MEMORY; 

  nsresult rv;
  nsCOMPtr<nsIDeviceContextSpecPS> psSpec = do_QueryInterface(mSpec, &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = mPSObj->Init(psSpec);
    if (NS_SUCCEEDED(rv))
      rv = nsPrintJobFactoryPS::CreatePrintJob(psSpec, mPrintJob);
  }
  if (NS_FAILED(rv)) {
    delete mPSObj;
    mPSObj = nsnull;
  }
  else {
    
    
    int num_copies;
    psSpec->GetCopies(num_copies);
    if (NS_FAILED(mPrintJob->SetNumCopies(num_copies)))
      mPSObj->SetNumCopies(num_copies);
  }

  return rv;
}

NS_IMPL_ISUPPORTS_INHERITED1(nsDeviceContextPS,
                             DeviceContextImpl,
                             nsIDeviceContextPS)





NS_IMETHODIMP
nsDeviceContextPS::InitDeviceContextPS(nsIDeviceContext *aCreatingDeviceContext,nsIDeviceContext *aParentContext)
{
  PR_LOG(nsDeviceContextPSLM, PR_LOG_DEBUG, ("nsDeviceContextPS::InitDeviceContextPS()\n"));

  float origscale, newscale;
  float t2d, a2d;

#ifdef WE_DO_NOT_SUPPORT_MULTIPLE_PRINT_DEVICECONTEXTS
  NS_ASSERTION(instance_counter < 2, "Cannot have more than one print device context.");
  if (instance_counter > 1) {
    return NS_ERROR_GFX_PRINTER_PRINT_WHILE_PREVIEW;
  }
#endif 

  NS_ENSURE_ARG_POINTER(aParentContext);

  mDepth = 24; 

  mTwipsToPixels = (float)72.0/(float)NSIntPointsToTwips(72);
  mPixelsToTwips = 1.0f / mTwipsToPixels;

  newscale = TwipsToDevUnits();
  origscale = aParentContext->TwipsToDevUnits();
  mCPixelScale = newscale / origscale;

  t2d = aParentContext->TwipsToDevUnits();
  a2d = aParentContext->AppUnitsToDevUnits();

  mAppUnitsToDevUnits = (a2d / t2d) * mTwipsToPixels;
  mDevUnitsToAppUnits = 1.0f / mAppUnitsToDevUnits;

  mParentDeviceContext = aParentContext;

  mPSFontGeneratorList = new nsHashtable();
  NS_ENSURE_TRUE(mPSFontGeneratorList, NS_ERROR_OUT_OF_MEMORY);
 
  nsresult rv;
  nsCOMPtr<nsIPref> pref(do_GetService(NS_PREF_CONTRACTID, &rv));
#ifdef MOZ_ENABLE_XFT
  if (NS_SUCCEEDED(rv)) {
      rv = pref->GetBoolPref("font.FreeType2.printing", &mFTPEnable);
      if (NS_FAILED(rv))
        mFTPEnable = PR_FALSE;
  }
#else 
  mFTPEnable = PR_FALSE;
#ifdef MOZ_ENABLE_FREETYPE2
  if (NS_SUCCEEDED(rv)) {
    rv = pref->GetBoolPref("font.FreeType2.enable", &mFTPEnable);
    if (NS_FAILED(rv))
      mFTPEnable = PR_FALSE;
    if (mFTPEnable) {
      rv = pref->GetBoolPref("font.FreeType2.printing", &mFTPEnable);
      if (NS_FAILED(rv))
        mFTPEnable = PR_FALSE;
    }
  }
#endif
#endif
  
  
  nsCOMPtr<nsILanguageAtomService> langService;
  langService = do_GetService(NS_LANGUAGEATOMSERVICE_CONTRACTID);
  if (langService) {
    NS_IF_ADDREF(gUsersLocale = langService->GetLocaleLanguageGroup());
  }
  if (!gUsersLocale) {
    gUsersLocale = NS_NewAtom("x-western");
  }

  return NS_OK;
}








NS_IMETHODIMP nsDeviceContextPS::CreateRenderingContext(nsIRenderingContext *&aContext)
{
  PR_LOG(nsDeviceContextPSLM, PR_LOG_DEBUG, ("nsDeviceContextPS::CreateRenderingContext()\n"));

  nsresult rv;
   
  aContext = nsnull;

  NS_ENSURE_TRUE(mPSObj != nsnull, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsRenderingContextPS> renderingContext = new nsRenderingContextPS();
  if (!renderingContext)
    return NS_ERROR_OUT_OF_MEMORY;
     
  rv = renderingContext->Init(this);

  if (NS_SUCCEEDED(rv)) {
    aContext = renderingContext;
    NS_ADDREF(aContext);
  }

  return rv;
}

NS_IMETHODIMP nsDeviceContextPS::CreateRenderingContextInstance(nsIRenderingContext *&aContext)
{
  nsCOMPtr<nsIRenderingContext> renderingContext = new nsRenderingContextPS();
  if (!renderingContext)
    return NS_ERROR_OUT_OF_MEMORY;
         
  aContext = renderingContext;
  NS_ADDREF(aContext);
  
  return NS_OK;
}





NS_IMETHODIMP nsDeviceContextPS::SupportsNativeWidgets(PRBool &aSupportsWidgets)
{
  PR_LOG(nsDeviceContextPSLM, PR_LOG_DEBUG, ("nsDeviceContextPS::SupportsNativeWidgets()\n"));

  aSupportsWidgets = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextPS::PrepareNativeWidget(nsIWidget* aWidget, void ** aOut)
{
  *aOut = nsnull;
  return NS_OK;
}





NS_IMETHODIMP nsDeviceContextPS::GetDepth(PRUint32& aDepth)
{
  PR_LOG(nsDeviceContextPSLM, PR_LOG_DEBUG, ("nsDeviceContextPS::GetDepth(mDepth=%d)\n", mDepth));

  return mDepth;
}





NS_IMETHODIMP nsDeviceContextPS::CheckFontExistence(const nsString& aFontName)
{
  PR_LOG(nsDeviceContextPSLM, PR_LOG_DEBUG, ("nsDeviceContextPS::CheckFontExistence()\n"));

  
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextPS::GetSystemFont(nsSystemFontID aID, nsFont *aFont) const
{
  PR_LOG(nsDeviceContextPSLM, PR_LOG_DEBUG, ("nsDeviceContextPS::GetSystemFont()\n"));

  if (mParentDeviceContext != nsnull) {
    return mParentDeviceContext->GetSystemFont(aID, aFont);
  }
  return NS_ERROR_FAILURE;
}





NS_IMETHODIMP nsDeviceContextPS::GetDeviceSurfaceDimensions(PRInt32 &aWidth, PRInt32 &aHeight)
{
  PR_LOG(nsDeviceContextPSLM, PR_LOG_DEBUG, ("nsDeviceContextPS::GetDeviceSurfaceDimensions()\n"));

  NS_ENSURE_TRUE(mPSObj && mPSObj->mPrintSetup, NS_ERROR_NULL_POINTER);

  
  aWidth  = mPSObj->mPrintSetup->width;
  aHeight = mPSObj->mPrintSetup->height;

  return NS_OK;
}




NS_IMETHODIMP nsDeviceContextPS::GetRect(nsRect &aRect)
{
  PR_LOG(nsDeviceContextPSLM, PR_LOG_DEBUG, ("nsDeviceContextPS::GetRect()\n"));

  NS_ENSURE_TRUE(mPSObj != nsnull, NS_ERROR_NULL_POINTER);

  PRInt32 width, height;
  nsresult rv;
  rv = GetDeviceSurfaceDimensions(width, height);
  aRect.x = 0;
  aRect.y = 0;
  aRect.width = width;
  aRect.height = height;
  return rv;
}




NS_IMETHODIMP nsDeviceContextPS::GetClientRect(nsRect &aRect)
{
  PR_LOG(nsDeviceContextPSLM, PR_LOG_DEBUG, ("nsDeviceContextPS::GetClientRect()\n"));

  return GetRect(aRect);
}





NS_IMETHODIMP nsDeviceContextPS::GetDeviceContextFor(nsIDeviceContextSpec *aDevice, nsIDeviceContext *&aContext)
{
  PR_LOG(nsDeviceContextPSLM, PR_LOG_DEBUG, ("nsDeviceContextPS::GetDeviceContextFor()\n"));
  aContext = nsnull;
  return NS_OK;
}





NS_IMETHODIMP nsDeviceContextPS::BeginDocument(PRUnichar * aTitle, PRUnichar* aPrintToFileName, PRInt32 aStartPage, PRInt32 aEndPage)
{
  PR_LOG(nsDeviceContextPSLM, PR_LOG_DEBUG, ("nsDeviceContextPS::BeginDocument()\n"));

  NS_ENSURE_TRUE(mPSObj != nsnull, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(mPrintJob != nsnull, NS_ERROR_NULL_POINTER);

  mPSObj->settitle(aTitle); 
  mPrintJob->SetJobTitle(aTitle);
  return NS_OK;
}

static PRBool PR_CALLBACK
GeneratePSFontCallback(nsHashKey *aKey, void *aData, void* aClosure)
{
  nsPSFontGenerator* psFontGenerator = (nsPSFontGenerator*)aData;
  NS_ENSURE_TRUE(psFontGenerator && aClosure, PR_FALSE);

  if (aClosure)
    psFontGenerator->GeneratePSFont((FILE *)aClosure);
  return PR_TRUE;
}





NS_IMETHODIMP nsDeviceContextPS::EndDocument(void)
{
  PR_LOG(nsDeviceContextPSLM, PR_LOG_DEBUG, ("nsDeviceContextPS::EndDocument()\n"));

  NS_ENSURE_TRUE(mPSObj != nsnull, NS_ERROR_NULL_POINTER);

  
  nsresult rv = mPSObj->end_document();
  if (NS_SUCCEEDED(rv)) {
    FILE *submitFP;
    rv = mPrintJob->StartSubmission(&submitFP);
    if (NS_ERROR_GFX_PRINTING_NOT_IMPLEMENTED == rv) {
      
      rv = NS_OK;
    }
    else if (NS_SUCCEEDED(rv)) {
      NS_ASSERTION(submitFP, "No print job submission handle");

      
#if defined(MOZ_ENABLE_FREETYPE2) || defined(MOZ_ENABLE_XFT)
      mPSObj->write_prolog(submitFP, mFTPEnable);
#else 
      mPSObj->write_prolog(submitFP);
#endif

      



      if (mPSFontGeneratorList)
        mPSFontGeneratorList->Enumerate(GeneratePSFontCallback,
            (void *) submitFP);

      rv = mPSObj->write_script(submitFP);
      if (NS_SUCCEEDED(rv))
        rv = mPrintJob->FinishSubmission();
    }
  }

  delete mPrintJob;
  mPrintJob = nsnull;

  PR_LOG(nsDeviceContextPSLM, PR_LOG_DEBUG,
      ("nsDeviceContextPS::EndDocument() return value %d\n", rv));

  return rv;
}





NS_IMETHODIMP nsDeviceContextPS::AbortDocument(void)
{
  PR_LOG(nsDeviceContextPSLM, PR_LOG_DEBUG, ("nsDeviceContextPS::AbortDocument()\n"));

  NS_ENSURE_TRUE(mPSObj != nsnull, NS_ERROR_NULL_POINTER);
  
  delete mPrintJob;
  mPrintJob = nsnull;
  return NS_OK;
}





NS_IMETHODIMP nsDeviceContextPS::BeginPage(void)
{
  PR_LOG(nsDeviceContextPSLM, PR_LOG_DEBUG, ("nsDeviceContextPS::BeginPage()\n"));

  NS_ENSURE_TRUE(mPSObj != nsnull, NS_ERROR_NULL_POINTER);

  
  mPSObj->begin_page(); 
  return NS_OK;
}





NS_IMETHODIMP nsDeviceContextPS::EndPage(void)
{
  PR_LOG(nsDeviceContextPSLM, PR_LOG_DEBUG, ("nsDeviceContextPS::EndPage()\n"));

  NS_ENSURE_TRUE(mPSObj != nsnull, NS_ERROR_NULL_POINTER);

  
  mPSObj->end_page();
  return NS_OK;
}

class nsFontCachePS : public nsFontCache
{
public:
  
  virtual nsresult CreateFontMetricsInstance(nsIFontMetrics** aResult);
};


nsresult nsFontCachePS::CreateFontMetricsInstance(nsIFontMetrics** aResult)
{
  NS_PRECONDITION(aResult, "null out param");
  nsIFontMetrics *fm = new nsFontMetricsPS();
  if (!fm)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(fm);
  *aResult = fm;
  return NS_OK;
}


NS_IMETHODIMP nsDeviceContextPS::CreateFontCache()
{
  PR_LOG(nsDeviceContextPSLM, PR_LOG_DEBUG, ("nsDeviceContextPS::CreateFontCache()\n"));

  mFontCache = new nsFontCachePS();
  if (!mFontCache) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  return mFontCache->Init(this);
}
