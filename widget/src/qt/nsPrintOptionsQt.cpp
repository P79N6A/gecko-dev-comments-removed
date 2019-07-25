





































#include "nsPrintSettingsQt.h"
#include "nsPrintOptionsQt.h"

nsPrintOptionsQt::nsPrintOptionsQt()
{
}

nsPrintOptionsQt::~nsPrintOptionsQt()
{
}

nsresult nsPrintOptionsQt::_CreatePrintSettings(nsIPrintSettings** _retval)
{
    nsPrintSettingsQt* printSettings = 
        new nsPrintSettingsQt(); 
    NS_ADDREF(*_retval = printSettings); 
    return NS_OK;
}
