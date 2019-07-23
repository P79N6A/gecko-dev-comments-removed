







































 


#define SET_PRINTER_FEATURES_VIA_PREFS 1 
#define PRINTERFEATURES_PREF "print.tmp.printerfeatures"

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG 1 /* Allow logging in the release build */
#endif 
#include "prlog.h"

#include "plstr.h"

#include "nsDeviceContextSpecG.h"

#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "prenv.h" 

#include "nsPrintfCString.h"
#include "nsReadableUtils.h"
#include "nsStringEnumerator.h"
#include "nsIServiceManager.h" 

#ifdef USE_POSTSCRIPT
#include "nsPSPrinters.h"
#include "nsPaperPS.h"  
#endif 

#include "nsPrintSettingsGTK.h"

#include "nsIFileStreams.h"
#include "nsILocalFile.h"
#include "nsTArray.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>


#define MAKE_PR_BOOL(val) ((val)?(PR_TRUE):(PR_FALSE))

#ifdef PR_LOGGING 
static PRLogModuleInfo *DeviceContextSpecGTKLM = PR_NewLogModule("DeviceContextSpecGTK");
#endif 

#define DO_PR_DEBUG_LOG(x) PR_LOG(DeviceContextSpecGTKLM, PR_LOG_DEBUG, x)







class GlobalPrinters {
public:
  static GlobalPrinters* GetInstance()   { return &mGlobalPrinters; }
  ~GlobalPrinters()                      { FreeGlobalPrinters(); }

  void      FreeGlobalPrinters();
  nsresult  InitializeGlobalPrinters();

  PRBool    PrintersAreAllocated()       { return mGlobalPrinterList != nsnull; }
  PRInt32   GetNumPrinters()
    { return mGlobalPrinterList ? mGlobalPrinterList->Length() : 0; }
  nsString* GetStringAt(PRInt32 aInx)    { return &mGlobalPrinterList->ElementAt(aInx); }
  void      GetDefaultPrinterName(PRUnichar **aDefaultPrinterName);

protected:
  GlobalPrinters() {}

  static GlobalPrinters mGlobalPrinters;
  static nsTArray<nsString>* mGlobalPrinterList;
};

#ifdef SET_PRINTER_FEATURES_VIA_PREFS

class nsPrinterFeatures {
public:
  nsPrinterFeatures( const char *printername );
  ~nsPrinterFeatures() {}

  
  void SetCanChangePaperSize( PRBool aCanSetPaperSize );
  
  void SetSupportsPaperSizeChange( PRBool aSupportsPaperSizeChange );
  
  void SetNumPaperSizeRecords( PRInt32 aCount );
  void SetPaperRecord( PRInt32 aIndex, const char *aName, PRInt32 aWidthMM, PRInt32 aHeightMM, PRBool aIsInch );

  
  void SetCanChangeOrientation( PRBool aCanSetOrientation );
  
  void SetSupportsOrientationChange( PRBool aSupportsOrientationChange );
  
  void SetNumOrientationRecords( PRInt32 aCount );
  void SetOrientationRecord( PRInt32 aIndex, const char *aName );

  
  void SetCanChangePlex( PRBool aCanSetPlex );
  
  void SetSupportsPlexChange( PRBool aSupportsPlexChange );
  
  void SetNumPlexRecords( PRInt32 aCount );
  void SetPlexRecord( PRInt32 aIndex, const char *aName );

  
  void SetCanChangeResolutionName( PRBool aCanSetResolutionName );
  
  void SetSupportsResolutionNameChange( PRBool aSupportsResolutionChange );
  
  void SetNumResolutionNameRecords( PRInt32 aCount );
  void SetResolutionNameRecord( PRInt32 aIndex, const char *aName );

  
  void SetCanChangeColorspace( PRBool aCanSetColorspace );
  
  void SetSupportsColorspaceChange( PRBool aSupportsColorspace );
  
  void SetNumColorspaceRecords( PRInt32 aCount );
  void SetColorspaceRecord( PRInt32 aIndex, const char *aName );

  
  void SetCanChangePrintInColor( PRBool aCanSetPrintInColor );
  
  void SetSupportsPrintInColorChange( PRBool aSupportPrintInColorChange );

  
  void SetCanChangeDownloadFonts( PRBool aCanSetDownloadFonts );
  
  void SetSupportsDownloadFontsChange( PRBool aSupportDownloadFontsChange );

  
  void SetCanChangeJobTitle( PRBool aCanSetJobTitle );
  
  void SetSupportsJobTitleChange( PRBool aSupportJobTitleChange );
    
  
  void SetCanChangeSpoolerCommand( PRBool aCanSetSpoolerCommand );
  
  void SetSupportsSpoolerCommandChange( PRBool aSupportSpoolerCommandChange );
  
  
  void SetCanChangeNumCopies( PRBool aCanSetNumCopies );

  


  void SetMultipleConcurrentDeviceContextsSupported( PRBool aCanUseMultipleInstances );
  
private:
  
  void SetBoolValue( const char *tagname, PRBool value );
  void SetIntValue(  const char *tagname, PRInt32 value );
  void SetCharValue(  const char *tagname, const char *value );

  nsXPIDLCString          mPrinterName;
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

void nsPrinterFeatures::SetSupportsPaperSizeChange( PRBool aSupportsPaperSizeChange )
{
  SetBoolValue("supports_paper_size_change", aSupportsPaperSizeChange);
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

void nsPrinterFeatures::SetSupportsOrientationChange( PRBool aSupportsOrientationChange )
{
  SetBoolValue("supports_orientation_change", aSupportsOrientationChange);
}

void nsPrinterFeatures::SetNumOrientationRecords( PRInt32 aCount )
{
  SetIntValue("orientation.count", aCount);          
}

void nsPrinterFeatures::SetOrientationRecord( PRInt32 aIndex, const char *aOrientationName )
{
  SetCharValue(nsPrintfCString(256, "orientation.%d.name", aIndex).get(), aOrientationName);
}

void nsPrinterFeatures::SetCanChangePlex( PRBool aCanSetPlex )
{
  SetBoolValue("can_change_plex", aCanSetPlex);
}

void nsPrinterFeatures::SetSupportsPlexChange( PRBool aSupportsPlexChange )
{
  SetBoolValue("supports_plex_change", aSupportsPlexChange);
}

void nsPrinterFeatures::SetNumPlexRecords( PRInt32 aCount )
{
  SetIntValue("plex.count", aCount);          
}

void nsPrinterFeatures::SetPlexRecord( PRInt32 aIndex, const char *aPlexName )
{
  SetCharValue(nsPrintfCString(256, "plex.%d.name", aIndex).get(), aPlexName);
}

void nsPrinterFeatures::SetCanChangeResolutionName( PRBool aCanSetResolutionName )
{
  SetBoolValue("can_change_resolution", aCanSetResolutionName);
}

void nsPrinterFeatures::SetSupportsResolutionNameChange( PRBool aSupportsResolutionNameChange )
{
  SetBoolValue("supports_resolution_change", aSupportsResolutionNameChange);
}

void nsPrinterFeatures::SetNumResolutionNameRecords( PRInt32 aCount )
{
  SetIntValue("resolution.count", aCount);          
}

void nsPrinterFeatures::SetResolutionNameRecord( PRInt32 aIndex, const char *aResolutionName )
{
  SetCharValue(nsPrintfCString(256, "resolution.%d.name", aIndex).get(), aResolutionName);
}

void nsPrinterFeatures::SetCanChangeColorspace( PRBool aCanSetColorspace )
{
  SetBoolValue("can_change_colorspace", aCanSetColorspace);
}

void nsPrinterFeatures::SetSupportsColorspaceChange( PRBool aSupportsColorspaceChange )
{
  SetBoolValue("supports_colorspace_change", aSupportsColorspaceChange);
}

void nsPrinterFeatures::SetNumColorspaceRecords( PRInt32 aCount )
{
  SetIntValue("colorspace.count", aCount);          
}

void nsPrinterFeatures::SetColorspaceRecord( PRInt32 aIndex, const char *aColorspace )
{
  SetCharValue(nsPrintfCString(256, "colorspace.%d.name", aIndex).get(), aColorspace);
}

void nsPrinterFeatures::SetCanChangeDownloadFonts( PRBool aCanSetDownloadFonts )
{
  SetBoolValue("can_change_downloadfonts", aCanSetDownloadFonts);
}

void nsPrinterFeatures::SetSupportsDownloadFontsChange( PRBool aSupportDownloadFontsChange )
{
  SetBoolValue("supports_downloadfonts_change", aSupportDownloadFontsChange);
}

void nsPrinterFeatures::SetCanChangePrintInColor( PRBool aCanSetPrintInColor )
{
  SetBoolValue("can_change_printincolor", aCanSetPrintInColor);
}

void nsPrinterFeatures::SetSupportsPrintInColorChange( PRBool aSupportPrintInColorChange )
{
  SetBoolValue("supports_printincolor_change", aSupportPrintInColorChange);
}

void nsPrinterFeatures::SetCanChangeSpoolerCommand( PRBool aCanSetSpoolerCommand )
{
  SetBoolValue("can_change_spoolercommand", aCanSetSpoolerCommand);
}

void nsPrinterFeatures::SetSupportsSpoolerCommandChange( PRBool aSupportSpoolerCommandChange )
{
  SetBoolValue("supports_spoolercommand_change", aSupportSpoolerCommandChange);
}

void nsPrinterFeatures::SetCanChangeJobTitle( PRBool aCanSetJobTitle )
{
  SetBoolValue("can_change_jobtitle", aCanSetJobTitle);
}

void nsPrinterFeatures::SetSupportsJobTitleChange( PRBool aSupportsJobTitle )
{
  SetBoolValue("supports_jobtitle_change", aSupportsJobTitle);
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
nsTArray<nsString>* GlobalPrinters::mGlobalPrinterList = nsnull;


nsDeviceContextSpecGTK::nsDeviceContextSpecGTK()
  : mPrintJob(NULL)
  , mGtkPrinter(NULL)
  , mGtkPrintSettings(NULL)
  , mGtkPageSetup(NULL)
{
  DO_PR_DEBUG_LOG(("nsDeviceContextSpecGTK::nsDeviceContextSpecGTK()\n"));
}

nsDeviceContextSpecGTK::~nsDeviceContextSpecGTK()
{
  DO_PR_DEBUG_LOG(("nsDeviceContextSpecGTK::~nsDeviceContextSpecGTK()\n"));

  if (mGtkPageSetup) {
    g_object_unref(mGtkPageSetup);
  }

  if (mGtkPrintSettings) {
    g_object_unref(mGtkPrintSettings);
  }
}

NS_IMPL_ISUPPORTS1(nsDeviceContextSpecGTK,
                   nsIDeviceContextSpec)

#include "gfxPDFSurface.h"
#include "gfxPSSurface.h"
NS_IMETHODIMP nsDeviceContextSpecGTK::GetSurfaceForPrinter(gfxASurface **aSurface)
{
  *aSurface = nsnull;

  const char *path;
  GetPath(&path);

  double width, height;
  mPrintSettings->GetEffectivePageSize(&width, &height);

  
  
  PRInt32 orientation;
  mPrintSettings->GetOrientation(&orientation);
  if (nsIPrintSettings::kLandscapeOrientation == orientation) {
    double tmp = width;
    width = height;
    height = tmp;
  }

  
  width  /= TWIPS_PER_POINT_FLOAT;
  height /= TWIPS_PER_POINT_FLOAT;

  DO_PR_DEBUG_LOG(("\"%s\", %f, %f\n", path, width, height));
  nsresult rv;

  
  
  gchar *buf;
  gint fd = g_file_open_tmp("XXXXXX.tmp", &buf, nsnull);
  if (-1 == fd)
    return NS_ERROR_GFX_PRINTER_COULD_NOT_OPEN_FILE;
  close(fd);

  rv = NS_NewNativeLocalFile(nsDependentCString(buf), PR_FALSE,
                             getter_AddRefs(mSpoolFile));
  if (NS_FAILED(rv)) {
    unlink(buf);
    return NS_ERROR_GFX_PRINTER_COULD_NOT_OPEN_FILE;
  }

  mSpoolName = buf;
  g_free(buf);

  mSpoolFile->SetPermissions(0600);

  nsCOMPtr<nsIFileOutputStream> stream = do_CreateInstance("@mozilla.org/network/file-output-stream;1");
  rv = stream->Init(mSpoolFile, -1, -1, 0);
  if (NS_FAILED(rv))
    return rv;

  PRInt16 format;
  mPrintSettings->GetOutputFormat(&format);

  nsRefPtr<gfxASurface> surface;
  gfxSize surfaceSize(width, height);

  
  if (format == nsIPrintSettings::kOutputFormatNative) {
    if (mIsPPreview) {
      
      format = nsIPrintSettings::kOutputFormatPS;
    } else {
      const gchar* fmtGTK = gtk_print_settings_get(mGtkPrintSettings, GTK_PRINT_SETTINGS_OUTPUT_FILE_FORMAT);
      if (!fmtGTK && GTK_IS_PRINTER(mGtkPrinter)) {
        
        format = (gtk_printer_accepts_ps(mGtkPrinter)) ? nsIPrintSettings::kOutputFormatPS
                                                       : nsIPrintSettings::kOutputFormatPDF;
      } else if (nsDependentCString(fmtGTK).EqualsIgnoreCase("pdf")) {
          format = nsIPrintSettings::kOutputFormatPDF;
      } else {
          format = nsIPrintSettings::kOutputFormatPS;
      }
    }
  }

  if (format == nsIPrintSettings::kOutputFormatPDF) {
    surface = new gfxPDFSurface(stream, surfaceSize);
  } else {
    surface = new gfxPSSurface(stream, surfaceSize);
  }

  if (!surface)
    return NS_ERROR_OUT_OF_MEMORY;

  surface.swap(*aSurface);

  return NS_OK;
}






NS_IMETHODIMP nsDeviceContextSpecGTK::Init(nsIWidget *aWidget,
                                           nsIPrintSettings* aPS,
                                           PRBool aIsPrintPreview)
{
  DO_PR_DEBUG_LOG(("nsDeviceContextSpecGTK::Init(aPS=%p)\n", aPS));

  if (gtk_major_version < 2 ||
      (gtk_major_version == 2 && gtk_minor_version < 10))
    return NS_ERROR_NOT_AVAILABLE;  

  mPrintSettings = aPS;
  mIsPPreview = aIsPrintPreview;

  
  PRBool toFile;
  aPS->GetPrintToFile(&toFile);

  mToPrinter = !toFile && !aIsPrintPreview;

  nsCOMPtr<nsPrintSettingsGTK> printSettingsGTK(do_QueryInterface(aPS));
  if (!printSettingsGTK)
    return NS_ERROR_NO_INTERFACE;

  mGtkPrinter = printSettingsGTK->GetGtkPrinter();
  mGtkPrintSettings = printSettingsGTK->GetGtkPrintSettings();
  mGtkPageSetup = printSettingsGTK->GetGtkPageSetup();

  
  
  
  GtkPaperSize* geckosHackishPaperSize = gtk_page_setup_get_paper_size(mGtkPageSetup);
  GtkPaperSize* standardGtkPaperSize = gtk_paper_size_new(gtk_paper_size_get_name(geckosHackishPaperSize));

  mGtkPageSetup = gtk_page_setup_copy(mGtkPageSetup);
  mGtkPrintSettings = gtk_print_settings_copy(mGtkPrintSettings);

  GtkPaperSize* properPaperSize;
  if (gtk_paper_size_is_equal(geckosHackishPaperSize, standardGtkPaperSize)) {
    properPaperSize = standardGtkPaperSize;
  } else {
    properPaperSize = geckosHackishPaperSize;
    gtk_paper_size_free(standardGtkPaperSize);
  }
  gtk_print_settings_set_paper_size(mGtkPrintSettings, properPaperSize);
  gtk_page_setup_set_paper_size_and_default_margins(mGtkPageSetup, properPaperSize);

  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecGTK::GetPath(const char **aPath)      
{
  *aPath = mPath;
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecGTK::GetPrintMethod(PrintMethod &aMethod)
{
  return GetPrintMethod(mPrinter, aMethod);
}

nsresult nsDeviceContextSpecGTK::GetPrintMethod(const char *aPrinter, PrintMethod &aMethod)
{
#if defined(USE_POSTSCRIPT)
  aMethod = pmPostScript;
  return NS_OK;
#else
  return NS_ERROR_UNEXPECTED;
#endif
}

static void
print_callback(GtkPrintJob *aJob, gpointer aData, GError *aError) {
  g_object_unref(aJob);
  ((nsILocalFile*) aData)->Remove(PR_FALSE);
}

static void
ns_release_macro(gpointer aData) {
  nsILocalFile* spoolFile = (nsILocalFile*) aData;
  NS_RELEASE(spoolFile);
}

NS_IMETHODIMP nsDeviceContextSpecGTK::BeginDocument(PRUnichar * aTitle, PRUnichar * aPrintToFileName,
                                                    PRInt32 aStartPage, PRInt32 aEndPage)
{
  if (mToPrinter) {
    if (!GTK_IS_PRINTER(mGtkPrinter))
      return NS_ERROR_FAILURE;

    mPrintJob = gtk_print_job_new(NS_ConvertUTF16toUTF8(aTitle).get(), mGtkPrinter,
                                  mGtkPrintSettings, mGtkPageSetup);
  }

  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecGTK::EndDocument()
{
  if (mToPrinter) {
    if (!mPrintJob)
      return NS_OK; 

    if (!gtk_print_job_set_source_file(mPrintJob, mSpoolName.get(), nsnull))
      return NS_ERROR_GFX_PRINTER_COULD_NOT_OPEN_FILE;

    NS_ADDREF(mSpoolFile.get());
    gtk_print_job_send(mPrintJob, print_callback, mSpoolFile, ns_release_macro);
  } else {
    
    nsXPIDLString targetPath;
    nsCOMPtr<nsILocalFile> destFile;
    mPrintSettings->GetToFileName(getter_Copies(targetPath));

    nsresult rv = NS_NewNativeLocalFile(NS_ConvertUTF16toUTF8(targetPath),
                                        PR_FALSE, getter_AddRefs(destFile));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString destLeafName;
    rv = destFile->GetLeafName(destLeafName);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFile> destDir;
    rv = destFile->GetParent(getter_AddRefs(destDir));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mSpoolFile->MoveTo(destDir, destLeafName);
    NS_ENSURE_SUCCESS(rv, rv);

    
    mode_t mask = umask(0);
    umask(mask);
    
    
    destFile->SetPermissions(0666 & ~(mask));
  }
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
        
        nsPrintfCString name(512, "print.%s.%s", modulename, prefname);
        DO_PR_DEBUG_LOG(("trying to get '%s'\n", name.get()));
        rv = pref->GetCharPref(name.get(), getter_Copies(return_buf));
      }
      
      if (NS_FAILED(rv)) {
        
        nsPrintfCString name(512, "print.%s", prefname);
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


nsPrinterEnumeratorGTK::nsPrinterEnumeratorGTK()
{
}

NS_IMPL_ISUPPORTS1(nsPrinterEnumeratorGTK, nsIPrinterEnumerator)

NS_IMETHODIMP nsPrinterEnumeratorGTK::GetPrinterNameList(nsIStringEnumerator **aPrinterNameList)
{
  NS_ENSURE_ARG_POINTER(aPrinterNameList);
  *aPrinterNameList = nsnull;
  
  nsresult rv = GlobalPrinters::GetInstance()->InitializeGlobalPrinters();
  if (NS_FAILED(rv)) {
    return rv;
  }

  PRInt32 numPrinters = GlobalPrinters::GetInstance()->GetNumPrinters();
  nsTArray<nsString> *printers = new nsTArray<nsString>(numPrinters);
  if (!printers) {
    GlobalPrinters::GetInstance()->FreeGlobalPrinters();
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  int count = 0;
  while( count < numPrinters )
  {
    printers->AppendElement(*GlobalPrinters::GetInstance()->GetStringAt(count++));
  }
  GlobalPrinters::GetInstance()->FreeGlobalPrinters();

  return NS_NewAdoptingStringEnumerator(aPrinterNameList, printers);
}


NS_IMETHODIMP nsPrinterEnumeratorGTK::GetDefaultPrinterName(PRUnichar **aDefaultPrinterName)
{
  DO_PR_DEBUG_LOG(("nsPrinterEnumeratorGTK::GetDefaultPrinterName()\n"));
  NS_ENSURE_ARG_POINTER(aDefaultPrinterName);

  GlobalPrinters::GetInstance()->GetDefaultPrinterName(aDefaultPrinterName);

  DO_PR_DEBUG_LOG(("GetDefaultPrinterName(): default printer='%s'.\n", NS_ConvertUTF16toUTF8(*aDefaultPrinterName).get()));
  return NS_OK;
}


NS_IMETHODIMP nsPrinterEnumeratorGTK::InitPrintSettingsFromPrinter(const PRUnichar *aPrinterName, nsIPrintSettings *aPrintSettings)
{
  DO_PR_DEBUG_LOG(("nsPrinterEnumeratorGTK::InitPrintSettingsFromPrinter()"));
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
  rv = nsDeviceContextSpecGTK::GetPrintMethod(printerName, type);
  if (NS_FAILED(rv))
    return rv;

#ifdef USE_POSTSCRIPT
  
  if (type == pmPostScript) {
    

    PRInt32 slash = printerName.FindChar('/');
    if (kNotFound != slash)
      printerName.Cut(0, slash + 1);
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

#ifdef USE_POSTSCRIPT
  if (type == pmPostScript) {
    DO_PR_DEBUG_LOG(("InitPrintSettingsFromPrinter() for PostScript printer\n"));

#ifdef SET_PRINTER_FEATURES_VIA_PREFS
    nsPrinterFeatures printerFeatures(fullPrinterName);

    printerFeatures.SetSupportsPaperSizeChange(PR_TRUE);
    printerFeatures.SetSupportsOrientationChange(PR_TRUE);
    printerFeatures.SetSupportsPlexChange(PR_FALSE);
    printerFeatures.SetSupportsResolutionNameChange(PR_FALSE);
    printerFeatures.SetSupportsColorspaceChange(PR_FALSE);
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
    printerFeatures.SetOrientationRecord(0, "portrait");
    printerFeatures.SetOrientationRecord(1, "landscape");
    printerFeatures.SetNumOrientationRecords(2);
#endif 

    
#ifdef SET_PRINTER_FEATURES_VIA_PREFS
    printerFeatures.SetCanChangePlex(PR_FALSE);
#endif 
    DO_PR_DEBUG_LOG(("setting default plex to '%s'\n", "default"));
    aPrintSettings->SetPlexName(NS_LITERAL_STRING("default").get());
#ifdef SET_PRINTER_FEATURES_VIA_PREFS
    printerFeatures.SetPlexRecord(0, "default");
    printerFeatures.SetNumPlexRecords(1);
#endif 

    
#ifdef SET_PRINTER_FEATURES_VIA_PREFS
    printerFeatures.SetCanChangeResolutionName(PR_FALSE);
#endif 
    DO_PR_DEBUG_LOG(("setting default resolution to '%s'\n", "default"));
    aPrintSettings->SetResolutionName(NS_LITERAL_STRING("default").get());
#ifdef SET_PRINTER_FEATURES_VIA_PREFS
    printerFeatures.SetResolutionNameRecord(0, "default");
    printerFeatures.SetNumResolutionNameRecords(1);
#endif 

    
#ifdef SET_PRINTER_FEATURES_VIA_PREFS
    printerFeatures.SetCanChangeColorspace(PR_FALSE);
#endif 
    DO_PR_DEBUG_LOG(("setting default colorspace to '%s'\n", "default"));
    aPrintSettings->SetColorspace(NS_LITERAL_STRING("default").get());
#ifdef SET_PRINTER_FEATURES_VIA_PREFS
    printerFeatures.SetColorspaceRecord(0, "default");
    printerFeatures.SetNumColorspaceRecords(1);
#endif    

#ifdef SET_PRINTER_FEATURES_VIA_PREFS
    printerFeatures.SetCanChangePaperSize(PR_TRUE);
#endif 
    nsXPIDLCString papername;
    if (NS_SUCCEEDED(CopyPrinterCharPref(pPrefs, "postscript", printerName, "paper_size", papername))) {
      nsPaperSizePS paper;

      if (paper.Find(papername)) {
        DO_PR_DEBUG_LOG(("setting default paper size to '%s' (%g mm/%g mm)\n",
              paper.Name(), paper.Width_mm(), paper.Height_mm()));
	aPrintSettings->SetPaperSizeUnit(nsIPrintSettings::kPaperSizeMillimeters);
        aPrintSettings->SetPaperWidth(paper.Width_mm());
        aPrintSettings->SetPaperHeight(paper.Height_mm());
        aPrintSettings->SetPaperName(NS_ConvertASCIItoUTF16(paper.Name()).get());
      }
      else {
        DO_PR_DEBUG_LOG(("Unknown paper size '%s' given.\n", papername.get()));
      }
#ifdef SET_PRINTER_FEATURES_VIA_PREFS
      paper.First();
      int count = 0;
      while (!paper.AtEnd())
      {
        printerFeatures.SetPaperRecord(count++, paper.Name(),
            (int)paper.Width_mm(), (int)paper.Height_mm(), !paper.IsMetric());
        paper.Next();
      }
      printerFeatures.SetNumPaperSizeRecords(count);
#endif 
    }

    PRBool hasSpoolerCmd = (nsPSPrinterList::kTypePS ==
        nsPSPrinterList::GetPrinterType(fullPrinterName));
#ifdef SET_PRINTER_FEATURES_VIA_PREFS
    printerFeatures.SetSupportsSpoolerCommandChange(hasSpoolerCmd);
    printerFeatures.SetCanChangeSpoolerCommand(hasSpoolerCmd);

    
    printerFeatures.SetSupportsJobTitleChange(PR_FALSE);
    printerFeatures.SetCanChangeJobTitle(PR_FALSE);
    
    printerFeatures.SetSupportsDownloadFontsChange(PR_FALSE);
    printerFeatures.SetCanChangeDownloadFonts(PR_FALSE);
    

    printerFeatures.SetSupportsPrintInColorChange(PR_TRUE);
    printerFeatures.SetCanChangePrintInColor(PR_TRUE);
#endif 

    if (hasSpoolerCmd) {
      nsXPIDLCString command;
      if (NS_SUCCEEDED(CopyPrinterCharPref(pPrefs, "postscript",
            printerName, "print_command", command))) {
        DO_PR_DEBUG_LOG(("setting default print command to '%s'\n",
            command.get()));
        aPrintSettings->SetPrintCommand(NS_ConvertUTF8toUTF16(command).get());
      }
    }
    
#ifdef SET_PRINTER_FEATURES_VIA_PREFS
    printerFeatures.SetCanChangeNumCopies(PR_TRUE);   
#endif 

    return NS_OK;    
  }
#endif 

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP nsPrinterEnumeratorGTK::DisplayPropertiesDlg(const PRUnichar *aPrinter, nsIPrintSettings *aPrintSettings)
{
  return NS_OK;
}


nsresult GlobalPrinters::InitializeGlobalPrinters ()
{
  if (PrintersAreAllocated()) {
    return NS_OK;
  }

  mGlobalPrinterList = new nsTArray<nsString>();
  if (!mGlobalPrinterList) 
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv;
  nsCOMPtr<nsIPrefBranch> pPrefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;
      
#ifdef USE_POSTSCRIPT
  nsPSPrinterList psMgr;
  if (NS_SUCCEEDED(psMgr.Init()) && psMgr.Enabled()) {
    
    
    
    
    nsTArray<nsCString> printerList;
    psMgr.GetPrinterList(printerList);
    for (PRUint32 i = 0; i < printerList.Length(); i++)
    {
      mGlobalPrinterList->AppendElement(NS_ConvertUTF8toUTF16(printerList[i]));
    }
  }
#endif   
      
  
  if (!mGlobalPrinterList->Length())
  {
    
    FreeGlobalPrinters();

    return NS_ERROR_GFX_PRINTER_NO_PRINTER_AVAILABLE;
  }

  return NS_OK;
}


void GlobalPrinters::FreeGlobalPrinters()
{
  if (mGlobalPrinterList) {
    delete mGlobalPrinterList;
    mGlobalPrinterList = nsnull;
  }  
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

