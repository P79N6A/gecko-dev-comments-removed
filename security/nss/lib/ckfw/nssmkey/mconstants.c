



#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: mconstants.c,v $ $Revision: 1.2 $ $Date: 2012/04/25 14:49:40 $";
#endif 







#ifndef NSSBASET_H
#include "nssbaset.h"
#endif 

#ifndef NSSCKT_H
#include "nssckt.h"
#endif 

#include "nssmkey.h"

NSS_IMPLEMENT_DATA const CK_VERSION
nss_ckmk_CryptokiVersion =  {
		NSS_CKMK_CRYPTOKI_VERSION_MAJOR,
		NSS_CKMK_CRYPTOKI_VERSION_MINOR };

NSS_IMPLEMENT_DATA const NSSUTF8 *
nss_ckmk_ManufacturerID = (NSSUTF8 *) "Mozilla Foundation";

NSS_IMPLEMENT_DATA const NSSUTF8 *
nss_ckmk_LibraryDescription = (NSSUTF8 *) "NSS Access to Mac OS X Key Ring";

NSS_IMPLEMENT_DATA const CK_VERSION
nss_ckmk_LibraryVersion = {
	NSS_CKMK_LIBRARY_VERSION_MAJOR,
	NSS_CKMK_LIBRARY_VERSION_MINOR};

NSS_IMPLEMENT_DATA const NSSUTF8 *
nss_ckmk_SlotDescription = (NSSUTF8 *) "Mac OS X Key Ring";

NSS_IMPLEMENT_DATA const CK_VERSION
nss_ckmk_HardwareVersion = { 
	NSS_CKMK_HARDWARE_VERSION_MAJOR,
	NSS_CKMK_HARDWARE_VERSION_MINOR };

NSS_IMPLEMENT_DATA const CK_VERSION
nss_ckmk_FirmwareVersion = { 
	NSS_CKMK_FIRMWARE_VERSION_MAJOR,
	NSS_CKMK_FIRMWARE_VERSION_MINOR };

NSS_IMPLEMENT_DATA const NSSUTF8 *
nss_ckmk_TokenLabel = (NSSUTF8 *) "Mac OS X Key Ring";

NSS_IMPLEMENT_DATA const NSSUTF8 *
nss_ckmk_TokenModel = (NSSUTF8 *) "1";

NSS_IMPLEMENT_DATA const NSSUTF8 *
nss_ckmk_TokenSerialNumber = (NSSUTF8 *) "1";

