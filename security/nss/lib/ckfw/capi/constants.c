



#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: constants.c,v $ $Revision: 1.2 $ $Date: 2012/04/25 14:49:30 $";
#endif 







#ifndef NSSBASET_H
#include "nssbaset.h"
#endif 

#ifndef NSSCKT_H
#include "nssckt.h"
#endif 

#ifndef NSSCAPI_H
#include "nsscapi.h"
#endif 

NSS_IMPLEMENT_DATA const CK_VERSION
nss_ckcapi_CryptokiVersion =  {
		NSS_CKCAPI_CRYPTOKI_VERSION_MAJOR,
		NSS_CKCAPI_CRYPTOKI_VERSION_MINOR };

NSS_IMPLEMENT_DATA const NSSUTF8 *
nss_ckcapi_ManufacturerID = (NSSUTF8 *) "Mozilla Foundation";

NSS_IMPLEMENT_DATA const NSSUTF8 *
nss_ckcapi_LibraryDescription = (NSSUTF8 *) "NSS Access to Microsoft Certificate Store";

NSS_IMPLEMENT_DATA const CK_VERSION
nss_ckcapi_LibraryVersion = {
	NSS_CKCAPI_LIBRARY_VERSION_MAJOR,
	NSS_CKCAPI_LIBRARY_VERSION_MINOR};

NSS_IMPLEMENT_DATA const NSSUTF8 *
nss_ckcapi_SlotDescription = (NSSUTF8 *) "Microsoft Certificate Store";

NSS_IMPLEMENT_DATA const CK_VERSION
nss_ckcapi_HardwareVersion = { 
	NSS_CKCAPI_HARDWARE_VERSION_MAJOR,
	NSS_CKCAPI_HARDWARE_VERSION_MINOR };

NSS_IMPLEMENT_DATA const CK_VERSION
nss_ckcapi_FirmwareVersion = { 
	NSS_CKCAPI_FIRMWARE_VERSION_MAJOR,
	NSS_CKCAPI_FIRMWARE_VERSION_MINOR };

NSS_IMPLEMENT_DATA const NSSUTF8 *
nss_ckcapi_TokenLabel = (NSSUTF8 *) "Microsoft Certificate Store";

NSS_IMPLEMENT_DATA const NSSUTF8 *
nss_ckcapi_TokenModel = (NSSUTF8 *) "1";

NSS_IMPLEMENT_DATA const NSSUTF8 *
nss_ckcapi_TokenSerialNumber = (NSSUTF8 *) "1";

