



#include "nsPrintOptionsGTK.h"
#include "nsPrintSettingsGTK.h"






nsPrintOptionsGTK::nsPrintOptionsGTK()
{

}





nsPrintOptionsGTK::~nsPrintOptionsGTK()
{
}


nsresult nsPrintOptionsGTK::_CreatePrintSettings(nsIPrintSettings **_retval)
{
  *_retval = nullptr;
  nsPrintSettingsGTK* printSettings = new nsPrintSettingsGTK(); 
  NS_ENSURE_TRUE(printSettings, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = printSettings); 

  return NS_OK;
}


