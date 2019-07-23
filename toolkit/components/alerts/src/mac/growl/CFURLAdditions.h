








#ifndef HAVE_CFURLADDITIONS_H
#define HAVE_CFURLADDITIONS_H

#include <CoreFoundation/CoreFoundation.h>
#include "CFGrowlDefines.h"


URL_TYPE createFileURLWithAliasData(DATA_TYPE aliasData);
DATA_TYPE createAliasDataWithURL(URL_TYPE theURL);


URL_TYPE createFileURLWithDockDescription(DICTIONARY_TYPE dict);

DICTIONARY_TYPE createDockDescriptionWithURL(URL_TYPE theURL);

#endif
