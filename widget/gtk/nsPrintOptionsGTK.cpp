




#include "nsPrintOptionsGTK.h"
#include "nsPrintSettingsGTK.h"

using namespace mozilla::embedding;





nsPrintOptionsGTK::nsPrintOptionsGTK()
{

}





nsPrintOptionsGTK::~nsPrintOptionsGTK()
{
}

static void
serialize_gtk_printsettings_to_printdata(const gchar *key,
                                         const gchar *value,
                                         gpointer aData)
{
  PrintData* data = (PrintData*)aData;
  CStringKeyValue pair;
  pair.key() = key;
  pair.value() = value;
  data->GTKPrintSettings().AppendElement(pair);
}

NS_IMETHODIMP
nsPrintOptionsGTK::SerializeToPrintData(nsIPrintSettings* aSettings,
                                        nsIWebBrowserPrint* aWBP,
                                        PrintData* data)
{
  nsresult rv = nsPrintOptions::SerializeToPrintData(aSettings, aWBP, data);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsPrintSettingsGTK> settingsGTK(do_QueryInterface(aSettings));
  NS_ENSURE_STATE(settingsGTK);

  GtkPrintSettings* gtkPrintSettings = settingsGTK->GetGtkPrintSettings();
  NS_ENSURE_STATE(gtkPrintSettings);

  gtk_print_settings_foreach(
    gtkPrintSettings,
    serialize_gtk_printsettings_to_printdata,
    data);

  return NS_OK;
}

NS_IMETHODIMP
nsPrintOptionsGTK::DeserializeToPrintSettings(const PrintData& data,
                                              nsIPrintSettings* settings)
{
  nsCOMPtr<nsPrintSettingsGTK> settingsGTK(do_QueryInterface(settings));
  NS_ENSURE_STATE(settingsGTK);

  nsresult rv = nsPrintOptions::DeserializeToPrintSettings(data, settings);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  GtkPrintSettings* newGtkPrintSettings = gtk_print_settings_new();

  for (uint32_t i = 0; i < data.GTKPrintSettings().Length(); ++i) {
    CStringKeyValue pair = data.GTKPrintSettings()[i];
    gtk_print_settings_set(newGtkPrintSettings,
                           pair.key().get(),
                           pair.value().get());
  }

  settingsGTK->SetGtkPrintSettings(newGtkPrintSettings);

  
  g_object_unref(newGtkPrintSettings);
  newGtkPrintSettings = nullptr;
  return NS_OK;
}


nsresult nsPrintOptionsGTK::_CreatePrintSettings(nsIPrintSettings **_retval)
{
  *_retval = nullptr;
  nsPrintSettingsGTK* printSettings = new nsPrintSettingsGTK(); 
  NS_ENSURE_TRUE(printSettings, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = printSettings); 

  return NS_OK;
}

