









































#define SET_PRINTER_FEATURES_VIA_PREFS 1
#define PRINTERFEATURES_PREF "print.tmp.printerfeatures"

#define FORCE_PR_LOG
#define PR_LOGGING 1
#include "prlog.h"

#include "nsDeviceContextSpecQt.h"

#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "prenv.h" 

#include "nsPrintfCString.h"
#include "nsReadableUtils.h"
#include "nsIServiceManager.h"
#include "nsCRT.h"

#ifdef USE_XPRINT
#include "xprintutil.h"
#endif 

#ifdef USE_POSTSCRIPT

#undef USE_POSTSCRIPT
#warning "fixme: postscript disabled"

#endif 


#define MAKE_PR_BOOL(val) ((val)?(PR_TRUE):(PR_FALSE))

#ifdef PR_LOGGING
static PRLogModuleInfo *DeviceContextSpecQtLM = PR_NewLogModule("DeviceContextSpecQt");
#endif 

#define DO_PR_DEBUG_LOG(x) PR_LOG(DeviceContextSpecQtLM, PR_LOG_DEBUG, x)







class GlobalPrinters {
public:
  static GlobalPrinters* GetInstance()   { return &mGlobalPrinters; }
  ~GlobalPrinters()                      { FreeGlobalPrinters(); }

  void      FreeGlobalPrinters();
  nsresult  InitializeGlobalPrinters();

  PRBool    PrintersAreAllocated()       { return mGlobalPrinterList != nsnull; }
  PRInt32   GetNumPrinters()             { return mGlobalNumPrinters; }
  nsString* GetStringAt(PRInt32 aInx)    { return mGlobalPrinterList->StringAt(aInx); }
  void      GetDefaultPrinterName(PRUnichar **aDefaultPrinterName);

protected:
  GlobalPrinters() {}

  static GlobalPrinters mGlobalPrinters;
  static nsStringArray* mGlobalPrinterList;
  static int            mGlobalNumPrinters;
};

#ifdef SET_PRINTER_FEATURES_VIA_PREFS

class nsPrinterFeatures {
public:
  nsPrinterFeatures( const char *printername );
  ~nsPrinterFeatures() {};

  
  void SetCanChangePaperSize( PRBool aCanSetPaperSize );
  
  void SetNumPaperSizeRecords( PRInt32 aCount );
  void SetPaperRecord( PRInt32 aIndex, const char *aName, PRInt32 aWidthMM, PRInt32 aHeightMM, PRBool aIsInch );

  
  void SetCanChangeOrientation( PRBool aCanSetOrientation );
  
  void SetNumOrientationRecords( PRInt32 aCount );
  void SetOrientationRecord( PRInt32 aIndex, const char *aName );

  
  void SetCanChangeSpoolerCommand( PRBool aCanSetSpoolerCommand );

  
  void SetCanChangeNumCopies( PRBool aCanSetNumCopies );

  


  void SetMultipleConcurrentDeviceContextsSupported( PRBool aCanUseMultipleInstances );

private:
  
  void SetBoolValue( const char *tagname, PRBool value );
  void SetIntValue(  const char *tagname, PRInt32 value );
  void SetCharValue(  const char *tagname, const char *value );

  nsCString    mPrinterName;
  nsCOMPtr<nsIPrefBranch> mPrefs;
};

void nsPrinterFeatures::SetBoolValue( const char *tagname, PRBool value )
{
  mPrefs->SetBoolPref(nsPrintfCString(256, PRINTERFEATURES_PREF ".%s.%s", mPrinterName.get(), tagname).get(), value);
}

void nsPrinterFeatures::SetIntValue(  const char *tagname, PRInt32 value )
{
  mPrefs->SetIntPref(nsPrintfCString(256, PRINTERFEATURES_PREF ".%s.%s", mPrinterName.get(), tagname).get(), value);
}

void nsPrinterFeatures::SetCharValue(  const char *tagname, const char *value )
{
  mPrefs->SetCharPref(nsPrintfCString(256, PRINTERFEATURES_PREF ".%s.%s", mPrinterName.get(), tagname).get(), value);
}

nsPrinterFeatures::nsPrinterFeatures( const char *printername )
{
  DO_PR_DEBUG_LOG(("nsPrinterFeatures::nsPrinterFeatures('%s')\n", printername));
  mPrinterName.Assign(printername);
  mPrefs = do_GetService(NS_PREFSERVICE_CONTRACTID);

  SetBoolValue("has_special_printerfeatures", PR_TRUE);
}

void nsPrinterFeatures::SetCanChangePaperSize( PRBool aCanSetPaperSize )
{
  SetBoolValue("can_change_paper_size", aCanSetPaperSize);
}


void nsPrinterFeatures::SetNumPaperSizeRecords( PRInt32 aCount )
{
  SetIntValue("paper.count", aCount);
}

void nsPrinterFeatures::SetPaperRecord(PRInt32 aIndex, const char *aPaperName, PRInt32 aWidthMM, PRInt32 aHeightMM, PRBool aIsInch)
{
  SetCharValue(nsPrintfCString(256, "paper.%d.name",      aIndex).get(), aPaperName);
  SetIntValue( nsPrintfCString(256, "paper.%d.width_mm",  aIndex).get(), aWidthMM);
  SetIntValue( nsPrintfCString(256, "paper.%d.height_mm", aIndex).get(), aHeightMM);
  SetBoolValue(nsPrintfCString(256, "paper.%d.is_inch",   aIndex).get(), aIsInch);
}

void nsPrinterFeatures::SetCanChangeOrientation( PRBool aCanSetOrientation )
{
  SetBoolValue("can_change_orientation", aCanSetOrientation);
}

void nsPrinterFeatures::SetNumOrientationRecords( PRInt32 aCount )
{
  SetIntValue("orientation.count", aCount);
}

void nsPrinterFeatures::SetOrientationRecord( PRInt32 aIndex, const char *aOrientationName )
{
  SetCharValue(nsPrintfCString(256, "orientation.%d.name", aIndex).get(), aOrientationName);
}

void nsPrinterFeatures::SetCanChangeSpoolerCommand( PRBool aCanSetSpoolerCommand )
{
  SetBoolValue("can_change_spoolercommand", aCanSetSpoolerCommand);
}

void nsPrinterFeatures::SetCanChangeNumCopies( PRBool aCanSetNumCopies )
{
  SetBoolValue("can_change_num_copies", aCanSetNumCopies);
}

void nsPrinterFeatures::SetMultipleConcurrentDeviceContextsSupported( PRBool aCanUseMultipleInstances )
{
  SetBoolValue("can_use_multiple_devicecontexts_concurrently", aCanUseMultipleInstances);
}

#endif 



GlobalPrinters GlobalPrinters::mGlobalPrinters;
nsStringArray* GlobalPrinters::mGlobalPrinterList = nsnull;
int            GlobalPrinters::mGlobalNumPrinters = 0;


nsDeviceContextSpecQt::nsDeviceContextSpecQt()
{
  DO_PR_DEBUG_LOG(("nsDeviceContextSpecQt::nsDeviceContextSpecQt()\n"));
}

nsDeviceContextSpecQt::~nsDeviceContextSpecQt()
{
  DO_PR_DEBUG_LOG(("nsDeviceContextSpecQt::~nsDeviceContextSpecQt()\n"));
}


#if defined(USE_XPRINT) && defined(USE_POSTSCRIPT)
NS_IMPL_ISUPPORTS3(nsDeviceContextSpecQt,
                   nsIDeviceContextSpec,
                   nsIDeviceContextSpecPS,
                   nsIDeviceContextSpecXp)

#elif !defined(USE_XPRINT) && defined(USE_POSTSCRIPT)
NS_IMPL_ISUPPORTS2(nsDeviceContextSpecQt,
                   nsIDeviceContextSpec,
                   nsIDeviceContextSpecPS)

#elif defined(USE_XPRINT) && !defined(USE_POSTSCRIPT)
NS_IMPL_ISUPPORTS2(nsDeviceContextSpecQt,
                   nsIDeviceContextSpec,
                   nsIDeviceContextSpecXp)

#elif !defined(USE_XPRINT) && !defined(USE_POSTSCRIPT)
NS_IMPL_ISUPPORTS1(nsDeviceContextSpecQt,
                   nsIDeviceContextSpec)
#else
#error "This should not happen"
#endif




















NS_IMETHODIMP nsDeviceContextSpecQt::Init(nsIWidget *aWidget,
                                          nsIPrintSettings* aPS,
                                          PRBool aIsPrintPreview)
{
  DO_PR_DEBUG_LOG(("nsDeviceContextSpecQt::Init(aPS=%p\n", aPS));
  nsresult rv = NS_ERROR_FAILURE;

  mPrintSettings = aPS;

  
  if (mPrintSettings) {
    PRBool isOn;
    mPrintSettings->GetPrintOptions(nsIPrintSettings::kEnableSelectionRB, &isOn);
    nsCOMPtr<nsIPref> pPrefs = do_GetService(NS_PREF_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
      (void) pPrefs->SetBoolPref("print.selection_radio_enabled", isOn);
    }
  }

  rv = GlobalPrinters::GetInstance()->InitializeGlobalPrinters();
  if (NS_FAILED(rv)) {
    return rv;
  }

  GlobalPrinters::GetInstance()->FreeGlobalPrinters();

  if (aPS) {
    PRBool     reversed       = PR_FALSE;
    PRBool     color          = PR_FALSE;
    PRBool     tofile         = PR_FALSE;
    PRInt16    printRange     = nsIPrintSettings::kRangeAllPages;
    PRInt32    orientation    = NS_PORTRAIT;
    PRInt32    fromPage       = 1;
    PRInt32    toPage         = 1;
    PRUnichar *command        = nsnull;
    PRInt32    copies         = 1;
    PRUnichar *printer        = nsnull;
    PRUnichar *papername      = nsnull;
    PRUnichar *printfile      = nsnull;
    double     dleft          = 0.5;
    double     dright         = 0.5;
    double     dtop           = 0.5;
    double     dbottom        = 0.5;

    aPS->GetPrinterName(&printer);
    aPS->GetPrintReversed(&reversed);
    aPS->GetPrintInColor(&color);
    aPS->GetPaperName(&papername);
    aPS->GetOrientation(&orientation);
    aPS->GetPrintCommand(&command);
    aPS->GetPrintRange(&printRange);
    aPS->GetToFileName(&printfile);
    aPS->GetPrintToFile(&tofile);
    aPS->GetStartPageRange(&fromPage);
    aPS->GetEndPageRange(&toPage);
    aPS->GetNumCopies(&copies);
    aPS->GetMarginTop(&dtop);
    aPS->GetMarginLeft(&dleft);
    aPS->GetMarginBottom(&dbottom);
    aPS->GetMarginRight(&dright);

    if (printfile)
      strcpy(mPath,    NS_ConvertUTF16toUTF8(printfile).get());
    if (command)
      strcpy(mCommand, NS_ConvertUTF16toUTF8(command).get());
    if (printer)
      strcpy(mPrinter, NS_ConvertUTF16toUTF8(printer).get());
    if (papername)
      strcpy(mPaperName, NS_ConvertUTF16toUTF8(papername).get());

    DO_PR_DEBUG_LOG(("margins:   %5.2f,%5.2f,%5.2f,%5.2f\n", dtop, dleft, dbottom, dright));
    DO_PR_DEBUG_LOG(("printRange %d\n",   printRange));
    DO_PR_DEBUG_LOG(("fromPage   %d\n",   fromPage));
    DO_PR_DEBUG_LOG(("toPage     %d\n",   toPage));
    DO_PR_DEBUG_LOG(("tofile     %d\n",   tofile));
    DO_PR_DEBUG_LOG(("printfile  '%s'\n", printfile? NS_ConvertUTF16toUTF8(printfile).get():"<NULL>"));
    DO_PR_DEBUG_LOG(("command    '%s'\n", command? NS_ConvertUTF16toUTF8(command).get():"<NULL>"));
    DO_PR_DEBUG_LOG(("printer    '%s'\n", printer? NS_ConvertUTF16toUTF8(printer).get():"<NULL>"));
    DO_PR_DEBUG_LOG(("papername  '%s'\n", papername? NS_ConvertUTF16toUTF8(papername).get():"<NULL>"));

    mTop         = dtop;
    mBottom      = dbottom;
    mLeft        = dleft;
    mRight       = dright;
    mFpf         = !reversed;
    mGrayscale   = !color;
    mOrientation = orientation;
    mToPrinter   = !tofile;
    mCopies      = copies;
  }

  return rv;
}

NS_IMETHODIMP nsDeviceContextSpecQt::GetToPrinter(PRBool &aToPrinter)
{
  aToPrinter = mToPrinter;
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecQt::GetPrinterName ( const char **aPrinter )
{
   *aPrinter = mPrinter;
   return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecQt::GetCopies ( int &aCopies )
{
   aCopies = mCopies;
   return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecQt::GetFirstPageFirst(PRBool &aFpf)
{
  aFpf = mFpf;
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecQt::GetGrayscale(PRBool &aGrayscale)
{
  aGrayscale = mGrayscale;
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecQt::GetLandscape(PRBool &aLandscape)
{
  aLandscape = (mOrientation == NS_LANDSCAPE);
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecQt::GetTopMargin(float &aValue)
{
  aValue = mTop;
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecQt::GetBottomMargin(float &aValue)
{
  aValue = mBottom;
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecQt::GetRightMargin(float &aValue)
{
  aValue = mRight;
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecQt::GetLeftMargin(float &aValue)
{
  aValue = mLeft;
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecQt::GetCommand(const char **aCommand)
{
  *aCommand = mCommand;
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecQt::GetPath(const char **aPath)
{
  *aPath = mPath;
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecQt::GetUserCancelled(PRBool &aCancel)
{
  aCancel = mCancel;
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecQt::GetPaperName( const char **aPaperName )
{
  *aPaperName = mPaperName;
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecQt::GetPageSizeInTwips(PRInt32 *aWidth, PRInt32 *aHeight)
{
  return mPrintSettings->GetPageSizeInTwips(aWidth, aHeight);
}

NS_IMETHODIMP nsDeviceContextSpecQt::GetPrintMethod(PrintMethod &aMethod)
{
  return GetPrintMethod(mPrinter, aMethod);
}


nsresult nsDeviceContextSpecQt::GetPrintMethod(const char *aPrinter, PrintMethod &aMethod)
{
#if defined(USE_POSTSCRIPT) && defined(USE_XPRINT)
  

  if (strncmp(aPrinter, NS_POSTSCRIPT_DRIVER_NAME,
              NS_POSTSCRIPT_DRIVER_NAME_LEN) != 0)
    aMethod = pmXprint;
  else
    aMethod = pmPostScript;
  return NS_OK;
#elif defined(USE_XPRINT)
  aMethod = pmXprint;
  return NS_OK;
#elif defined(USE_POSTSCRIPT)
  aMethod = pmPostScript;
  return NS_OK;
#else
  return NS_ERROR_UNEXPECTED;
#endif
}

NS_IMETHODIMP nsDeviceContextSpecQt::ClosePrintManager()
{
  return NS_OK;
}








static
nsresult CopyPrinterCharPref(nsIPrefBranch *pref, const char *modulename, const char *printername,
                             const char *prefname, nsXPIDLCString &return_buf)
{
  DO_PR_DEBUG_LOG(("CopyPrinterCharPref('%s', '%s', '%s')\n", modulename, printername, prefname));

  nsresult rv = NS_ERROR_FAILURE;

  if (printername && modulename) {
    
    nsPrintfCString name(512, "print.%s.printer_%s.%s", modulename, printername, prefname);
    DO_PR_DEBUG_LOG(("trying to get '%s'\n", name.get()));
    rv = pref->GetCharPref(name.get(), getter_Copies(return_buf));
  }

  if (NS_FAILED(rv)) {
    if (printername) {
      
      nsPrintfCString name(512, "print.printer_%s.%s", printername, prefname);
      DO_PR_DEBUG_LOG(("trying to get '%s'\n", name.get()));
      rv = pref->GetCharPref(name.get(), getter_Copies(return_buf));
    }

    if (NS_FAILED(rv)) {
      if (modulename) {
        
        nsPrintfCString name(512, "print.printer_%s.%s", printername, prefname);
        DO_PR_DEBUG_LOG(("trying to get '%s'\n", name.get()));
        rv = pref->GetCharPref(name.get(), getter_Copies(return_buf));
      }

      if (NS_FAILED(rv)) {
        
        nsPrintfCString name(512, "print.%s.%s", modulename, prefname);
        DO_PR_DEBUG_LOG(("trying to get '%s'\n", name.get()));
        rv = pref->GetCharPref(name.get(), getter_Copies(return_buf));
      }
    }
  }

#ifdef PR_LOG
  if (NS_SUCCEEDED(rv)) {
    DO_PR_DEBUG_LOG(("CopyPrinterCharPref returning '%s'.\n", return_buf.get()));
  }
  else
  {
    DO_PR_DEBUG_LOG(("CopyPrinterCharPref failure.\n"));
  }
#endif 

  return rv;
}


nsPrinterEnumeratorQt::nsPrinterEnumeratorQt()
{
}

NS_IMPL_ISUPPORTS1(nsPrinterEnumeratorQt, nsIPrinterEnumerator)

NS_IMETHODIMP nsPrinterEnumeratorQt::EnumeratePrinters(PRUint32* aCount, PRUnichar*** aResult)
{
  NS_ENSURE_ARG(aCount);
  NS_ENSURE_ARG_POINTER(aResult);

  if (aCount)
    *aCount = 0;
  else
    return NS_ERROR_NULL_POINTER;

  if (aResult)
    *aResult = nsnull;
  else
    return NS_ERROR_NULL_POINTER;

  nsresult rv = GlobalPrinters::GetInstance()->InitializeGlobalPrinters();
  if (NS_FAILED(rv)) {
    return rv;
  }

  PRInt32 numPrinters = GlobalPrinters::GetInstance()->GetNumPrinters();

  PRUnichar** array = (PRUnichar**) nsMemory::Alloc(numPrinters * sizeof(PRUnichar*));
  if (!array && numPrinters > 0) {
    GlobalPrinters::GetInstance()->FreeGlobalPrinters();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  int count = 0;
  while( count < numPrinters )
  {
    PRUnichar *str = ToNewUnicode(*GlobalPrinters::GetInstance()->GetStringAt(count));

    if (!str) {
      for (int i = count - 1; i >= 0; i--)
        nsMemory::Free(array[i]);

      nsMemory::Free(array);

      GlobalPrinters::GetInstance()->FreeGlobalPrinters();
      return NS_ERROR_OUT_OF_MEMORY;
    }
    array[count++] = str;

  }
  *aCount = count;
  *aResult = array;
  GlobalPrinters::GetInstance()->FreeGlobalPrinters();

  return NS_OK;
}


NS_IMETHODIMP nsPrinterEnumeratorQt::GetDefaultPrinterName(PRUnichar **aDefaultPrinterName)
{
  DO_PR_DEBUG_LOG(("nsPrinterEnumeratorQt::GetDefaultPrinterName()\n"));
  NS_ENSURE_ARG_POINTER(aDefaultPrinterName);

  GlobalPrinters::GetInstance()->GetDefaultPrinterName(aDefaultPrinterName);

  DO_PR_DEBUG_LOG(("GetDefaultPrinterName(): default printer='%s'.\n", NS_ConvertUTF16toUTF8(*aDefaultPrinterName).get()));
  return NS_OK;
}


NS_IMETHODIMP nsPrinterEnumeratorQt::InitPrintSettingsFromPrinter(const PRUnichar *aPrinterName, nsIPrintSettings *aPrintSettings)
{
  DO_PR_DEBUG_LOG(("nsPrinterEnumeratorQt::InitPrintSettingsFromPrinter()"));
  nsresult rv;

  NS_ENSURE_ARG_POINTER(aPrinterName);
  NS_ENSURE_ARG_POINTER(aPrintSettings);

  NS_ENSURE_TRUE(*aPrinterName, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(aPrintSettings, NS_ERROR_FAILURE);

  nsCOMPtr<nsIPrefBranch> pPrefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;

  nsXPIDLCString fullPrinterName, 
                 printerName;     
  fullPrinterName.Assign(NS_ConvertUTF16toUTF8(aPrinterName));
  printerName.Assign(NS_ConvertUTF16toUTF8(aPrinterName));
  DO_PR_DEBUG_LOG(("printerName='%s'\n", printerName.get()));

  PrintMethod type = pmInvalid;
  rv = nsDeviceContextSpecQt::GetPrintMethod(printerName, type);
  if (NS_FAILED(rv))
    return rv;

#ifdef USE_POSTSCRIPT
  
  if (type == pmPostScript) {
    

    printerName.Cut(0, NS_POSTSCRIPT_DRIVER_NAME_LEN);
  }
#endif 

#ifdef SET_PRINTER_FEATURES_VIA_PREFS
  
  pPrefs->SetBoolPref(nsPrintfCString(256, PRINTERFEATURES_PREF ".%s.has_special_printerfeatures", fullPrinterName.get()).get(), PR_FALSE);
#endif 


  
  nsXPIDLCString filename;
  if (NS_FAILED(CopyPrinterCharPref(pPrefs, nsnull, printerName, "filename", filename))) {
    const char *path;

    if (!(path = PR_GetEnv("PWD")))
      path = PR_GetEnv("HOME");

    if (path)
      filename = nsPrintfCString(PATH_MAX, "%s/mozilla.ps", path);
    else
      filename.AssignLiteral("mozilla.ps");
  }
  DO_PR_DEBUG_LOG(("Setting default filename to '%s'\n", filename.get()));
  aPrintSettings->SetToFileName(NS_ConvertUTF8toUTF16(filename).get());

  aPrintSettings->SetIsInitializedFromPrinter(PR_TRUE);
#ifdef USE_XPRINT
  if (type == pmXprint) {
    DO_PR_DEBUG_LOG(("InitPrintSettingsFromPrinter() for Xprint printer\n"));

    Display   *pdpy;
    XPContext  pcontext;
    if (XpuGetPrinter(printerName, &pdpy, &pcontext) != 1)
      return NS_ERROR_GFX_PRINTER_NAME_NOT_FOUND;

    XpuSupportedFlags supported_doc_attrs = XpuGetSupportedDocAttributes(pdpy, pcontext);

#ifdef SET_PRINTER_FEATURES_VIA_PREFS
    nsPrinterFeatures printerFeatures(fullPrinterName);
#endif 

    
    XpuOrientationList  olist;
    int                 ocount;
    XpuOrientationRec  *default_orientation;

#ifdef SET_PRINTER_FEATURES_VIA_PREFS
    PRBool canSetOrientation = MAKE_PR_BOOL(supported_doc_attrs & XPUATTRIBUTESUPPORTED_CONTENT_ORIENTATION);
    printerFeatures.SetCanChangeOrientation(canSetOrientation);
#endif 

    
    olist = XpuGetOrientationList(pdpy, pcontext, &ocount);
    if (olist) {
      default_orientation = &olist[0]; 

      if (!PL_strcasecmp(default_orientation->orientation, "portrait")) {
        DO_PR_DEBUG_LOG(("setting default orientation to 'portrait'\n"));
        aPrintSettings->SetOrientation(nsIPrintSettings::kPortraitOrientation);
      }
      else if (!PL_strcasecmp(default_orientation->orientation, "landscape")) {
        DO_PR_DEBUG_LOG(("setting default orientation to 'landscape'\n"));
        aPrintSettings->SetOrientation(nsIPrintSettings::kLandscapeOrientation);
      }
      else {
        DO_PR_DEBUG_LOG(("Unknown default orientation '%s'\n", default_orientation->orientation));
      }

#ifdef SET_PRINTER_FEATURES_VIA_PREFS
      int i;
      for( i = 0 ; i < ocount ; i++ )
      {
        XpuOrientationRec *curr = &olist[i];
        printerFeatures.SetOrientationRecord(i, curr->orientation);
      }
      printerFeatures.SetNumOrientationRecords(ocount);
#endif 

      XpuFreeOrientationList(olist);
    }

    
#ifdef SET_PRINTER_FEATURES_VIA_PREFS
    PRBool canSetNumCopies = MAKE_PR_BOOL(supported_doc_attrs & XPUATTRIBUTESUPPORTED_COPY_COUNT);
    printerFeatures.SetCanChangeNumCopies(canSetNumCopies);
#endif 
    long numCopies;
    if( XpuGetOneLongAttribute(pdpy, pcontext, XPDocAttr, "copy-count", &numCopies) != 1 )
    {
      
      numCopies = 1;
    }
    aPrintSettings->SetNumCopies(numCopies);

    
    XpuMediumSourceSizeList mlist;
    int                     mcount;
    XpuMediumSourceSizeRec *default_medium;

#ifdef SET_PRINTER_FEATURES_VIA_PREFS
    PRBool canSetPaperSize = MAKE_PR_BOOL(supported_doc_attrs & XPUATTRIBUTESUPPORTED_DEFAULT_MEDIUM);
    printerFeatures.SetCanChangePaperSize(canSetPaperSize);
#endif 

    mlist = XpuGetMediumSourceSizeList(pdpy, pcontext, &mcount);
    if (mlist) {
      nsCAutoString papername;

      default_medium = &mlist[0]; 
      double total_width  = default_medium->ma1 + default_medium->ma2,
             total_height = default_medium->ma3 + default_medium->ma4;

      
      if (default_medium->tray_name) {
        papername = nsPrintfCString(256, "%s/%s", default_medium->tray_name, default_medium->medium_name);
      }
      else {
        papername.Assign(default_medium->medium_name);
      }

      DO_PR_DEBUG_LOG(("setting default paper size to '%s' (%g/%g mm)\n", papername.get(), total_width, total_height));
      aPrintSettings->SetPaperSizeType(nsIPrintSettings::kPaperSizeDefined);
      aPrintSettings->SetPaperSizeUnit(nsIPrintSettings::kPaperSizeMillimeters);
      aPrintSettings->SetPaperWidth(total_width);
      aPrintSettings->SetPaperHeight(total_height);
      aPrintSettings->SetPaperName(NS_ConvertUTF8toUTF16(papername).get());

#ifdef SET_PRINTER_FEATURES_VIA_PREFS
      int i;
      for( i = 0 ; i < mcount ; i++ )
      {
        XpuMediumSourceSizeRec *curr = &mlist[i];
        double total_width  = curr->ma1 + curr->ma2,
               total_height = curr->ma3 + curr->ma4;
        if (curr->tray_name) {
          papername = nsPrintfCString(256, "%s/%s", curr->tray_name, curr->medium_name);
        }
        else {
          papername.Assign(curr->medium_name);
        }

        printerFeatures.SetPaperRecord(i, papername.get(), PRInt32(total_width), PRInt32(total_height), PR_FALSE);
      }
      printerFeatures.SetNumPaperSizeRecords(mcount);
#endif 

      XpuFreeMediumSourceSizeList(mlist);
    }

#ifdef SET_PRINTER_FEATURES_VIA_PREFS
    

    printerFeatures.SetCanChangeSpoolerCommand(PR_FALSE);

    

    printerFeatures.SetMultipleConcurrentDeviceContextsSupported(PR_TRUE);
#endif 

    XpuClosePrinterDisplay(pdpy, pcontext);

    return NS_OK;
  }
  else
#endif 

#ifdef USE_POSTSCRIPT
  if (type == pmPostScript) {
    DO_PR_DEBUG_LOG(("InitPrintSettingsFromPrinter() for PostScript printer\n"));

#ifdef SET_PRINTER_FEATURES_VIA_PREFS
    nsPrinterFeatures printerFeatures(fullPrinterName);
#endif 

#ifdef SET_PRINTER_FEATURES_VIA_PREFS
    printerFeatures.SetCanChangeOrientation(PR_TRUE);
#endif 

    nsXPIDLCString orientation;
    if (NS_SUCCEEDED(CopyPrinterCharPref(pPrefs, "postscript", printerName, "orientation", orientation))) {
      if (orientation.LowerCaseEqualsLiteral("portrait")) {
        DO_PR_DEBUG_LOG(("setting default orientation to 'portrait'\n"));
        aPrintSettings->SetOrientation(nsIPrintSettings::kPortraitOrientation);
      }
      else if (orientation.LowerCaseEqualsLiteral("landscape")) {
        DO_PR_DEBUG_LOG(("setting default orientation to 'landscape'\n"));
        aPrintSettings->SetOrientation(nsIPrintSettings::kLandscapeOrientation);
      }
      else {
        DO_PR_DEBUG_LOG(("Unknown default orientation '%s'\n", orientation.get()));
      }
    }

#ifdef SET_PRINTER_FEATURES_VIA_PREFS
    int i;
    for( i = 0 ; postscript_module_orientations[i].orientation != nsnull ; i++ )
    {
      const PSOrientationRec *curr = &postscript_module_orientations[i];
      printerFeatures.SetOrientationRecord(i, curr->orientation);
    }
    printerFeatures.SetNumOrientationRecords(i);
#endif 

#ifdef SET_PRINTER_FEATURES_VIA_PREFS
    printerFeatures.SetCanChangePaperSize(PR_TRUE);
#endif 
    nsXPIDLCString papername;
    if (NS_SUCCEEDED(CopyPrinterCharPref(pPrefs, "postscript", printerName, "paper_size", papername))) {
      int                   i;
      const PSPaperSizeRec *default_paper = nsnull;

      for( i = 0 ; postscript_module_paper_sizes[i].name != nsnull ; i++ )
      {
        const PSPaperSizeRec *curr = &postscript_module_paper_sizes[i];

        if (!PL_strcasecmp(papername, curr->name)) {
          default_paper = curr;
          break;
        }
      }

      if (default_paper) {
        DO_PR_DEBUG_LOG(("setting default paper size to '%s' (%g inch/%g inch)\n",
                        default_paper->name,
                        PSPaperSizeRec_FullPaperWidth(default_paper),
                        PSPaperSizeRec_FullPaperHeight(default_paper)));
        aPrintSettings->SetPaperSizeType(nsIPrintSettings::kPaperSizeDefined);
        aPrintSettings->SetPaperSizeUnit(nsIPrintSettings::kPaperSizeInches);
        aPrintSettings->SetPaperWidth(PSPaperSizeRec_FullPaperWidth(default_paper));
        aPrintSettings->SetPaperHeight(PSPaperSizeRec_FullPaperHeight(default_paper));
        aPrintSettings->SetPaperName(NS_ConvertUTF8toUTF16(default_paper->name).get());
      }
      else {
        DO_PR_DEBUG_LOG(("Unknown paper size '%s' given.\n", papername.get()));
      }
#ifdef SET_PRINTER_FEATURES_VIA_PREFS
      for( i = 0 ; postscript_module_paper_sizes[i].name != nsnull ; i++ )
      {
        const PSPaperSizeRec *curr = &postscript_module_paper_sizes[i];
#define CONVERT_INCH_TO_MILLIMETERS(inch) ((inch) * 25.4)
        double total_width  = CONVERT_INCH_TO_MILLIMETERS(PSPaperSizeRec_FullPaperWidth(curr)),
               total_height = CONVERT_INCH_TO_MILLIMETERS(PSPaperSizeRec_FullPaperHeight(curr));

        printerFeatures.SetPaperRecord(i, curr->name, PRInt32(total_width), PRInt32(total_height), PR_TRUE);
      }
      printerFeatures.SetNumPaperSizeRecords(i);
#endif 
    }

#ifdef SET_PRINTER_FEATURES_VIA_PREFS
    printerFeatures.SetCanChangeSpoolerCommand(PR_TRUE);
#endif 

    nsXPIDLCString command;
    if (NS_SUCCEEDED(CopyPrinterCharPref(pPrefs, "postscript", printerName, "print_command", command))) {
      DO_PR_DEBUG_LOG(("setting default print command to '%s'\n", command.get()));
      aPrintSettings->SetPrintCommand(NS_ConvertUTF8toUTF16(command).get());
    }

#ifdef SET_PRINTER_FEATURES_VIA_PREFS
    printerFeatures.SetCanChangeNumCopies(PR_TRUE);
#endif 

    return NS_OK;
  }
#endif 

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP nsPrinterEnumeratorQt::DisplayPropertiesDlg(const PRUnichar *aPrinter, nsIPrintSettings *aPrintSettings)
{
  return NS_OK;
}


nsresult GlobalPrinters::InitializeGlobalPrinters ()
{
  if (PrintersAreAllocated()) {
    return NS_OK;
  }

  mGlobalNumPrinters = 0;
  mGlobalPrinterList = new nsStringArray();
  if (!mGlobalPrinterList)
    return NS_ERROR_OUT_OF_MEMORY;

#ifdef USE_XPRINT
  XPPrinterList plist = XpuGetPrinterList(nsnull, &mGlobalNumPrinters);

  if (plist && (mGlobalNumPrinters > 0))
  {
    int i;
    for(  i = 0 ; i < mGlobalNumPrinters ; i++ )
    {
      mGlobalPrinterList->AppendString(nsString(NS_ConvertASCIItoUTF16(plist[i].name)));
    }

    XpuFreePrinterList(plist);
  }
#endif 

#ifdef USE_POSTSCRIPT
  
  char   *printerList           = nsnull;
  PRBool  added_default_printer = PR_FALSE; 

  
  printerList = PR_GetEnv("MOZILLA_POSTSCRIPT_PRINTER_LIST");

  if (!printerList) {
    nsresult rv;
    nsCOMPtr<nsIPrefBranch> pPrefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
      (void) pPrefs->CopyCharPref("print.printer_list", &printerList);
    }
  }

  if (printerList) {
    char       *tok_lasts;
    const char *name;

    
    printerList = strdup(printerList);
    if (!printerList)
      return NS_ERROR_OUT_OF_MEMORY;

    for( name = PL_strtok_r(printerList, " ", &tok_lasts) ;
         name != nsnull ;
         name = PL_strtok_r(nsnull, " ", &tok_lasts) )
    {
      
      if (!strcmp(name, "default"))
        added_default_printer = PR_TRUE;

      mGlobalPrinterList->AppendString(
        nsString(NS_ConvertASCIItoUTF16(NS_POSTSCRIPT_DRIVER_NAME)) +
        nsString(NS_ConvertASCIItoUTF16(name)));
      mGlobalNumPrinters++;
    }

    free(printerList);
  }

  

  if (!added_default_printer)
  {
    mGlobalPrinterList->AppendString(
      nsString(NS_ConvertASCIItoUTF16(NS_POSTSCRIPT_DRIVER_NAME "default")));
    mGlobalNumPrinters++;
  }
#endif 

  if (mGlobalNumPrinters == 0)
    return NS_ERROR_GFX_PRINTER_NO_PRINTER_AVAILABLE;

  return NS_OK;
}


void GlobalPrinters::FreeGlobalPrinters()
{
  delete mGlobalPrinterList;
  mGlobalPrinterList = nsnull;
  mGlobalNumPrinters = 0;
}

void
GlobalPrinters::GetDefaultPrinterName(PRUnichar **aDefaultPrinterName)
{
  *aDefaultPrinterName = nsnull;

  PRBool allocate = (GlobalPrinters::GetInstance()->PrintersAreAllocated() == PR_FALSE);

  if (allocate) {
    nsresult rv = GlobalPrinters::GetInstance()->InitializeGlobalPrinters();
    if (NS_FAILED(rv)) {
      return;
    }
  }
  NS_ASSERTION(GlobalPrinters::GetInstance()->PrintersAreAllocated(), "no GlobalPrinters");

  if (GlobalPrinters::GetInstance()->GetNumPrinters() == 0)
    return;

  *aDefaultPrinterName = ToNewUnicode(*GlobalPrinters::GetInstance()->GetStringAt(0));

  if (allocate) {
    GlobalPrinters::GetInstance()->FreeGlobalPrinters();
  }
}

