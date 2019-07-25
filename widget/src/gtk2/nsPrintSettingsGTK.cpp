




































#include "nsPrintSettingsGTK.h"
#include "nsILocalFile.h"
#include "nsNetUtil.h"
#include <stdlib.h>

static
gboolean ref_printer(GtkPrinter *aPrinter, gpointer aData)
{
  ((nsPrintSettingsGTK*) aData)->SetGtkPrinter(aPrinter);
  return TRUE;
}

static
gboolean printer_enumerator(GtkPrinter *aPrinter, gpointer aData)
{
  if (gtk_printer_is_default(aPrinter))
    return ref_printer(aPrinter, aData);

  return FALSE; 
}

static
GtkPaperSize* moz_gtk_paper_size_copy_to_new_custom(GtkPaperSize* oldPaperSize)
{
  
  return gtk_paper_size_new_custom(gtk_paper_size_get_name(oldPaperSize),
                                   gtk_paper_size_get_display_name(oldPaperSize),
                                   gtk_paper_size_get_width(oldPaperSize, GTK_UNIT_INCH),
                                   gtk_paper_size_get_height(oldPaperSize, GTK_UNIT_INCH),
                                   GTK_UNIT_INCH);
}

NS_IMPL_ISUPPORTS_INHERITED1(nsPrintSettingsGTK, 
                             nsPrintSettings, 
                             nsPrintSettingsGTK)



nsPrintSettingsGTK::nsPrintSettingsGTK() :
  mPageSetup(NULL),
  mPrintSettings(NULL),
  mGTKPrinter(NULL),
  mPrintSelectionOnly(PR_FALSE)
{
  
  
  mPrintSettings = gtk_print_settings_new();
  mPageSetup = gtk_page_setup_new();
  InitUnwriteableMargin();

  SetOutputFormat(nsIPrintSettings::kOutputFormatNative);

  GtkPaperSize* defaultPaperSize = gtk_paper_size_new(NULL);
  mPaperSize = moz_gtk_paper_size_copy_to_new_custom(defaultPaperSize);
  gtk_paper_size_free(defaultPaperSize);
  SaveNewPageSize();
}



nsPrintSettingsGTK::~nsPrintSettingsGTK()
{
  if (mPageSetup) {
    g_object_unref(mPageSetup);
    mPageSetup = NULL;
  }
  if (mPrintSettings) {
    g_object_unref(mPrintSettings);
    mPrintSettings = NULL;
  }
  if (mGTKPrinter) {
    g_object_unref(mGTKPrinter);
    mGTKPrinter = NULL;
  }
  gtk_paper_size_free(mPaperSize);
}



nsPrintSettingsGTK::nsPrintSettingsGTK(const nsPrintSettingsGTK& aPS) :
  mPageSetup(NULL),
  mPrintSettings(NULL),
  mGTKPrinter(NULL),
  mPrintSelectionOnly(PR_FALSE)
{
  *this = aPS;
}



nsPrintSettingsGTK& nsPrintSettingsGTK::operator=(const nsPrintSettingsGTK& rhs)
{
  if (this == &rhs) {
    return *this;
  }
  
  nsPrintSettings::operator=(rhs);

  if (mPageSetup)
    g_object_unref(mPageSetup);
  mPageSetup = gtk_page_setup_copy(rhs.mPageSetup);
  
  
  

  if (mPrintSettings)
    g_object_unref(mPrintSettings);
  mPrintSettings = gtk_print_settings_copy(rhs.mPrintSettings);

  if (mGTKPrinter)
    g_object_unref(mGTKPrinter);
  mGTKPrinter = (GtkPrinter*) g_object_ref(rhs.mGTKPrinter);

  mPrintSelectionOnly = rhs.mPrintSelectionOnly;

  return *this;
}



nsresult nsPrintSettingsGTK::_Clone(nsIPrintSettings **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsnull;
  
  nsPrintSettingsGTK *newSettings = new nsPrintSettingsGTK(*this);
  if (!newSettings)
    return NS_ERROR_FAILURE;
  *_retval = newSettings;
  NS_ADDREF(*_retval);
  return NS_OK;
}




NS_IMETHODIMP
nsPrintSettingsGTK::_Assign(nsIPrintSettings *aPS)
{
  nsPrintSettingsGTK *printSettingsGTK = static_cast<nsPrintSettingsGTK*>(aPS);
  if (!printSettingsGTK)
    return NS_ERROR_UNEXPECTED;
  *this = *printSettingsGTK;
  return NS_OK;
}



void
nsPrintSettingsGTK::SetGtkPageSetup(GtkPageSetup *aPageSetup)
{
  if (mPageSetup)
    g_object_unref(mPageSetup);
  
  mPageSetup = (GtkPageSetup*) g_object_ref(aPageSetup);
  InitUnwriteableMargin();

  
  
  GtkPaperSize* newPaperSize = gtk_page_setup_get_paper_size(aPageSetup);
  if (newPaperSize) { 
    gtk_paper_size_free(mPaperSize);
    mPaperSize = moz_gtk_paper_size_copy_to_new_custom(newPaperSize);
  }
  
  
  SaveNewPageSize();
}



void
nsPrintSettingsGTK::SetGtkPrintSettings(GtkPrintSettings *aPrintSettings)
{
  if (mPrintSettings)
    g_object_unref(mPrintSettings);
  
  mPrintSettings = (GtkPrintSettings*) g_object_ref(aPrintSettings);

  GtkPaperSize* newPaperSize = gtk_print_settings_get_paper_size(aPrintSettings);
  if (newPaperSize) {
    gtk_paper_size_free(mPaperSize);
    mPaperSize = moz_gtk_paper_size_copy_to_new_custom(newPaperSize);
  }
  SaveNewPageSize();
}



void
nsPrintSettingsGTK::SetGtkPrinter(GtkPrinter *aPrinter)
{
  if (mGTKPrinter)
    g_object_unref(mGTKPrinter);
  
  mGTKPrinter = (GtkPrinter*) g_object_ref(aPrinter);
}







NS_IMETHODIMP nsPrintSettingsGTK::GetPrintRange(PRInt16 *aPrintRange)
{
  NS_ENSURE_ARG_POINTER(aPrintRange);
  if (mPrintSelectionOnly) {
    *aPrintRange = kRangeSelection;
    return NS_OK;
  }

  GtkPrintPages gtkRange = gtk_print_settings_get_print_pages(mPrintSettings);
  if (gtkRange == GTK_PRINT_PAGES_RANGES)
    *aPrintRange = kRangeSpecifiedPageRange;
  else
    *aPrintRange = kRangeAllPages;

  return NS_OK;
}
NS_IMETHODIMP nsPrintSettingsGTK::SetPrintRange(PRInt16 aPrintRange)
{
  if (aPrintRange == kRangeSelection) {
    mPrintSelectionOnly = PR_TRUE;
    return NS_OK;
  }

  mPrintSelectionOnly = PR_FALSE;
  if (aPrintRange == kRangeSpecifiedPageRange)
    gtk_print_settings_set_print_pages(mPrintSettings, GTK_PRINT_PAGES_RANGES);
  else
    gtk_print_settings_set_print_pages(mPrintSettings, GTK_PRINT_PAGES_ALL);
  return NS_OK;
}


NS_IMETHODIMP
nsPrintSettingsGTK::GetStartPageRange(PRInt32 *aStartPageRange)
{
  gint ctRanges;
  GtkPageRange* lstRanges = gtk_print_settings_get_page_ranges(mPrintSettings, &ctRanges);

  
  if (ctRanges < 1) {
    *aStartPageRange = 1;
  } else {
    
    
    PRInt32 start(lstRanges[0].start);
    for (gint ii = 1; ii < ctRanges; ii++) {
      start = NS_MIN(lstRanges[ii].start, start);
    }
    *aStartPageRange = start + 1;
  }

  g_free(lstRanges);
  return NS_OK;
}
NS_IMETHODIMP
nsPrintSettingsGTK::SetStartPageRange(PRInt32 aStartPageRange)
{
  PRInt32 endRange;
  GetEndPageRange(&endRange);

  GtkPageRange gtkRange;
  gtkRange.start = aStartPageRange - 1;
  gtkRange.end = endRange - 1;

  gtk_print_settings_set_page_ranges(mPrintSettings, &gtkRange, 1);

  return NS_OK;
}


NS_IMETHODIMP
nsPrintSettingsGTK::GetEndPageRange(PRInt32 *aEndPageRange)
{
  gint ctRanges;
  GtkPageRange* lstRanges = gtk_print_settings_get_page_ranges(mPrintSettings, &ctRanges);

  if (ctRanges < 1) {
    *aEndPageRange = 1;
  } else {
    PRInt32 end(lstRanges[0].end);
    for (gint ii = 1; ii < ctRanges; ii++) {
      end = NS_MAX(lstRanges[ii].end, end);
    }
    *aEndPageRange = end + 1;
  }

  g_free(lstRanges);
  return NS_OK;
}
NS_IMETHODIMP
nsPrintSettingsGTK::SetEndPageRange(PRInt32 aEndPageRange)
{
  PRInt32 startRange;
  GetStartPageRange(&startRange);

  GtkPageRange gtkRange;
  gtkRange.start = startRange - 1;
  gtkRange.end = aEndPageRange - 1;

  gtk_print_settings_set_page_ranges(mPrintSettings, &gtkRange, 1);

  return NS_OK;
}


NS_IMETHODIMP
nsPrintSettingsGTK::GetPrintReversed(PRBool *aPrintReversed)
{
  *aPrintReversed = gtk_print_settings_get_reverse(mPrintSettings);
  return NS_OK;
}
NS_IMETHODIMP
nsPrintSettingsGTK::SetPrintReversed(PRBool aPrintReversed)
{
  gtk_print_settings_set_reverse(mPrintSettings, aPrintReversed);
  return NS_OK;
}


NS_IMETHODIMP
nsPrintSettingsGTK::GetPrintInColor(PRBool *aPrintInColor)
{
  *aPrintInColor = gtk_print_settings_get_use_color(mPrintSettings);
  return NS_OK;
}
NS_IMETHODIMP
nsPrintSettingsGTK::SetPrintInColor(PRBool aPrintInColor)
{
  gtk_print_settings_set_use_color(mPrintSettings, aPrintInColor);
  return NS_OK;
}


NS_IMETHODIMP
nsPrintSettingsGTK::GetOrientation(PRInt32 *aOrientation)
{
  NS_ENSURE_ARG_POINTER(aOrientation);

  GtkPageOrientation gtkOrient = gtk_page_setup_get_orientation(mPageSetup);
  switch (gtkOrient) {
    case GTK_PAGE_ORIENTATION_LANDSCAPE:
    case GTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE:
      *aOrientation = kLandscapeOrientation;
      break;

    case GTK_PAGE_ORIENTATION_PORTRAIT:
    case GTK_PAGE_ORIENTATION_REVERSE_PORTRAIT:
    default:
      *aOrientation = kPortraitOrientation;
  }
  return NS_OK;
}
NS_IMETHODIMP
nsPrintSettingsGTK::SetOrientation(PRInt32 aOrientation)
{
  GtkPageOrientation gtkOrient;
  if (aOrientation == kLandscapeOrientation)
    gtkOrient = GTK_PAGE_ORIENTATION_LANDSCAPE;
  else
    gtkOrient = GTK_PAGE_ORIENTATION_PORTRAIT;

  gtk_print_settings_set_orientation(mPrintSettings, gtkOrient);
  gtk_page_setup_set_orientation(mPageSetup, gtkOrient);
  return NS_OK;
}


NS_IMETHODIMP
nsPrintSettingsGTK::GetToFileName(PRUnichar * *aToFileName)
{
  
  const char* gtk_output_uri = gtk_print_settings_get(mPrintSettings, GTK_PRINT_SETTINGS_OUTPUT_URI);
  if (!gtk_output_uri) {
    *aToFileName = ToNewUnicode(mToFileName);
    return NS_OK;
  }

  
  nsCOMPtr<nsIFile> file;
  nsresult rv = NS_GetFileFromURLSpec(nsDependentCString(gtk_output_uri),
                                      getter_AddRefs(file));
  if (NS_FAILED(rv))
    return rv;

  
  nsAutoString path;
  rv = file->GetPath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  *aToFileName = ToNewUnicode(path);
  return NS_OK;
}

NS_IMETHODIMP
nsPrintSettingsGTK::SetToFileName(const PRUnichar * aToFileName)
{
  if (aToFileName[0] == 0) {
    mToFileName.SetLength(0);
    gtk_print_settings_set(mPrintSettings, GTK_PRINT_SETTINGS_OUTPUT_URI, NULL);
    return NS_OK;
  }

  if (StringEndsWith(nsDependentString(aToFileName), NS_LITERAL_STRING(".ps"))) {
    gtk_print_settings_set(mPrintSettings, GTK_PRINT_SETTINGS_OUTPUT_FILE_FORMAT, "ps");
  } else {
    gtk_print_settings_set(mPrintSettings, GTK_PRINT_SETTINGS_OUTPUT_FILE_FORMAT, "pdf");
  }

  nsCOMPtr<nsILocalFile> file;
  nsresult rv = NS_NewLocalFile(nsDependentString(aToFileName), PR_TRUE,
                                getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCAutoString url;
  rv = NS_GetURLSpecFromFile(file, url);
  NS_ENSURE_SUCCESS(rv, rv);

  gtk_print_settings_set(mPrintSettings, GTK_PRINT_SETTINGS_OUTPUT_URI, url.get());
  mToFileName = aToFileName;

  return NS_OK;
}

NS_IMETHODIMP
nsPrintSettingsGTK::GetPrinterName(PRUnichar * *aPrinter)
{
  const char* gtkPrintName = gtk_print_settings_get_printer(mPrintSettings);
  if (!gtkPrintName) {
    if (GTK_IS_PRINTER(mGTKPrinter)) {
      gtkPrintName = gtk_printer_get_name(mGTKPrinter);
    } else {
      
      nsXPIDLString nullPrintName;
      *aPrinter = ToNewUnicode(nullPrintName);
      return NS_OK;
    }
  }
  *aPrinter = ToNewUnicode(nsDependentCString(gtkPrintName));
  return NS_OK;
}

NS_IMETHODIMP
nsPrintSettingsGTK::SetPrinterName(const PRUnichar * aPrinter)
{
  NS_ConvertUTF16toUTF8 gtkPrinter(aPrinter);

  if (StringBeginsWith(gtkPrinter, NS_LITERAL_CSTRING("CUPS/"))) {
    
    gtkPrinter.Cut(0, strlen("CUPS/"));
  }

  
  
  
  
  const char* oldPrinterName = gtk_print_settings_get_printer(mPrintSettings);
  if (!oldPrinterName || !gtkPrinter.Equals(oldPrinterName)) {
    mIsInitedFromPrinter = PR_FALSE;
    mIsInitedFromPrefs = PR_FALSE;
    gtk_print_settings_set_printer(mPrintSettings, gtkPrinter.get());
  }

  return NS_OK;
}


NS_IMETHODIMP
nsPrintSettingsGTK::GetNumCopies(PRInt32 *aNumCopies)
{
  NS_ENSURE_ARG_POINTER(aNumCopies);
  *aNumCopies = gtk_print_settings_get_n_copies(mPrintSettings);
  return NS_OK;
}
NS_IMETHODIMP
nsPrintSettingsGTK::SetNumCopies(PRInt32 aNumCopies)
{
  gtk_print_settings_set_n_copies(mPrintSettings, aNumCopies);
  return NS_OK;
}


NS_IMETHODIMP
nsPrintSettingsGTK::GetScaling(double *aScaling)
{
  *aScaling = gtk_print_settings_get_scale(mPrintSettings) / 100.0;
  return NS_OK;
}

NS_IMETHODIMP
nsPrintSettingsGTK::SetScaling(double aScaling)
{
  gtk_print_settings_set_scale(mPrintSettings, aScaling * 100.0);
  return NS_OK;
}


NS_IMETHODIMP
nsPrintSettingsGTK::GetPaperName(PRUnichar * *aPaperName)
{
  NS_ENSURE_ARG_POINTER(aPaperName);
  *aPaperName = ToNewUnicode(NS_ConvertUTF8toUTF16(gtk_paper_size_get_name(mPaperSize)));
  return NS_OK;
}
NS_IMETHODIMP
nsPrintSettingsGTK::SetPaperName(const PRUnichar * aPaperName)
{
  NS_ConvertUTF16toUTF8 gtkPaperName(aPaperName);

  
  if (gtkPaperName.EqualsIgnoreCase("letter"))
    gtkPaperName.AssignLiteral(GTK_PAPER_NAME_LETTER);
  else if (gtkPaperName.EqualsIgnoreCase("legal"))
    gtkPaperName.AssignLiteral(GTK_PAPER_NAME_LEGAL);

  
  GtkPaperSize* paperSize = gtk_paper_size_new(gtkPaperName.get());
  char* displayName = strdup(gtk_paper_size_get_display_name(paperSize));
  gtk_paper_size_free(paperSize);

  paperSize = gtk_paper_size_new_custom(gtkPaperName.get(), displayName,
                                        gtk_paper_size_get_width(mPaperSize, GTK_UNIT_INCH),
                                        gtk_paper_size_get_height(mPaperSize, GTK_UNIT_INCH),
                                        GTK_UNIT_INCH);

  free(displayName);
  gtk_paper_size_free(mPaperSize);
  mPaperSize = paperSize;
  SaveNewPageSize();
  return NS_OK;
}

GtkUnit
nsPrintSettingsGTK::GetGTKUnit(PRInt16 aGeckoUnit)
{
  if (aGeckoUnit == kPaperSizeMillimeters)
    return GTK_UNIT_MM;
  else
    return GTK_UNIT_INCH;
}

void
nsPrintSettingsGTK::SaveNewPageSize()
{
  gtk_print_settings_set_paper_size(mPrintSettings, mPaperSize);
  gtk_page_setup_set_paper_size(mPageSetup, mPaperSize);
}

void
nsPrintSettingsGTK::InitUnwriteableMargin()
{
  mUnwriteableMargin.SizeTo(
   NS_INCHES_TO_INT_TWIPS(gtk_page_setup_get_left_margin(mPageSetup, GTK_UNIT_INCH)),
   NS_INCHES_TO_INT_TWIPS(gtk_page_setup_get_top_margin(mPageSetup, GTK_UNIT_INCH)),
   NS_INCHES_TO_INT_TWIPS(gtk_page_setup_get_right_margin(mPageSetup, GTK_UNIT_INCH)),
   NS_INCHES_TO_INT_TWIPS(gtk_page_setup_get_bottom_margin(mPageSetup, GTK_UNIT_INCH))
  );
}













NS_IMETHODIMP 
nsPrintSettingsGTK::SetUnwriteableMarginInTwips(nsIntMargin& aUnwriteableMargin)
{
  nsPrintSettings::SetUnwriteableMarginInTwips(aUnwriteableMargin);
  gtk_page_setup_set_top_margin(mPageSetup,
           NS_TWIPS_TO_INCHES(mUnwriteableMargin.top), GTK_UNIT_INCH);
  gtk_page_setup_set_left_margin(mPageSetup,
           NS_TWIPS_TO_INCHES(mUnwriteableMargin.left), GTK_UNIT_INCH);
  gtk_page_setup_set_bottom_margin(mPageSetup,
           NS_TWIPS_TO_INCHES(mUnwriteableMargin.bottom), GTK_UNIT_INCH);
  gtk_page_setup_set_right_margin(mPageSetup,
           NS_TWIPS_TO_INCHES(mUnwriteableMargin.right), GTK_UNIT_INCH);
  return NS_OK;
}


NS_IMETHODIMP
nsPrintSettingsGTK::SetUnwriteableMarginTop(double aUnwriteableMarginTop)
{
  nsPrintSettings::SetUnwriteableMarginTop(aUnwriteableMarginTop);
  gtk_page_setup_set_top_margin(mPageSetup,
           NS_TWIPS_TO_INCHES(mUnwriteableMargin.top), GTK_UNIT_INCH);
  return NS_OK;
}


NS_IMETHODIMP
nsPrintSettingsGTK::SetUnwriteableMarginLeft(double aUnwriteableMarginLeft)
{
  nsPrintSettings::SetUnwriteableMarginLeft(aUnwriteableMarginLeft);
  gtk_page_setup_set_left_margin(mPageSetup,
           NS_TWIPS_TO_INCHES(mUnwriteableMargin.left), GTK_UNIT_INCH);
  return NS_OK;
}


NS_IMETHODIMP
nsPrintSettingsGTK::SetUnwriteableMarginBottom(double aUnwriteableMarginBottom)
{
  nsPrintSettings::SetUnwriteableMarginBottom(aUnwriteableMarginBottom);
  gtk_page_setup_set_bottom_margin(mPageSetup,
           NS_TWIPS_TO_INCHES(mUnwriteableMargin.bottom), GTK_UNIT_INCH);
  return NS_OK;
}


NS_IMETHODIMP
nsPrintSettingsGTK::SetUnwriteableMarginRight(double aUnwriteableMarginRight)
{
  nsPrintSettings::SetUnwriteableMarginRight(aUnwriteableMarginRight);
  gtk_page_setup_set_right_margin(mPageSetup,
           NS_TWIPS_TO_INCHES(mUnwriteableMargin.right), GTK_UNIT_INCH);
  return NS_OK;
}


NS_IMETHODIMP
nsPrintSettingsGTK::GetPaperWidth(double *aPaperWidth)
{
  NS_ENSURE_ARG_POINTER(aPaperWidth);
  *aPaperWidth = gtk_paper_size_get_width(mPaperSize, GetGTKUnit(mPaperSizeUnit));
  return NS_OK;
}
NS_IMETHODIMP
nsPrintSettingsGTK::SetPaperWidth(double aPaperWidth)
{
  gtk_paper_size_set_size(mPaperSize,
                          aPaperWidth,
                          gtk_paper_size_get_height(mPaperSize, GetGTKUnit(mPaperSizeUnit)),
                          GetGTKUnit(mPaperSizeUnit));
  SaveNewPageSize();
  return NS_OK;
}


NS_IMETHODIMP
nsPrintSettingsGTK::GetPaperHeight(double *aPaperHeight)
{
  NS_ENSURE_ARG_POINTER(aPaperHeight);
  *aPaperHeight = gtk_paper_size_get_height(mPaperSize, GetGTKUnit(mPaperSizeUnit));
  return NS_OK;
}
NS_IMETHODIMP
nsPrintSettingsGTK::SetPaperHeight(double aPaperHeight)
{
  gtk_paper_size_set_size(mPaperSize,
                          gtk_paper_size_get_width(mPaperSize, GetGTKUnit(mPaperSizeUnit)),
                          aPaperHeight,
                          GetGTKUnit(mPaperSizeUnit));
  SaveNewPageSize();
  return NS_OK;
}

NS_IMETHODIMP
nsPrintSettingsGTK::SetPaperSizeUnit(PRInt16 aPaperSizeUnit)
{
  
  
  gtk_paper_size_set_size(mPaperSize,
                          gtk_paper_size_get_width(mPaperSize, GetGTKUnit(mPaperSizeUnit)),
                          gtk_paper_size_get_height(mPaperSize, GetGTKUnit(mPaperSizeUnit)),
                          GetGTKUnit(aPaperSizeUnit));
  SaveNewPageSize();

  mPaperSizeUnit = aPaperSizeUnit;
  return NS_OK;
}

NS_IMETHODIMP
nsPrintSettingsGTK::GetEffectivePageSize(double *aWidth, double *aHeight)
{
  *aWidth  = NS_INCHES_TO_INT_TWIPS(gtk_paper_size_get_width(mPaperSize, GTK_UNIT_INCH));
  *aHeight = NS_INCHES_TO_INT_TWIPS(gtk_paper_size_get_height(mPaperSize, GTK_UNIT_INCH));

  GtkPageOrientation gtkOrient = gtk_page_setup_get_orientation(mPageSetup);

  if (gtkOrient == GTK_PAGE_ORIENTATION_LANDSCAPE ||
      gtkOrient == GTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE) {
    double temp = *aWidth;
    *aWidth = *aHeight;
    *aHeight = temp;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsPrintSettingsGTK::SetupSilentPrinting()
{
  
  
  
  gtk_enumerate_printers(printer_enumerator, this, NULL, TRUE);

  
  if (!GTK_IS_PRINTER(mGTKPrinter))
    gtk_enumerate_printers(ref_printer, this, NULL, TRUE);

  return NS_OK;
}
