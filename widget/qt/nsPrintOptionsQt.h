







































#ifndef nsPrintOptionsQt_h__
#define nsPrintOptionsQt_h__

#include "nsPrintOptionsImpl.h"

class nsPrintOptionsQt : public nsPrintOptions
{
public:
    nsPrintOptionsQt();
    virtual ~nsPrintOptionsQt();
    virtual nsresult _CreatePrintSettings(nsIPrintSettings** _retval);
};

#endif 
