



#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: constants.c,v $ $Revision: 1.14 $ $Date: 2012/04/25 14:49:29 $";
#endif 







#ifndef NSSBASET_H
#include "nssbaset.h"
#endif 

#ifndef NSSCKT_H
#include "nssckt.h"
#endif 

#ifndef NSSCKBI_H
#include "nssckbi.h"
#endif 

const CK_VERSION
nss_builtins_CryptokiVersion =  {
		NSS_BUILTINS_CRYPTOKI_VERSION_MAJOR,
		NSS_BUILTINS_CRYPTOKI_VERSION_MINOR };

const CK_VERSION
nss_builtins_LibraryVersion = {
	NSS_BUILTINS_LIBRARY_VERSION_MAJOR,
	NSS_BUILTINS_LIBRARY_VERSION_MINOR};

const CK_VERSION
nss_builtins_HardwareVersion = { 
	NSS_BUILTINS_HARDWARE_VERSION_MAJOR,
	NSS_BUILTINS_HARDWARE_VERSION_MINOR };

const CK_VERSION
nss_builtins_FirmwareVersion = { 
	NSS_BUILTINS_FIRMWARE_VERSION_MAJOR,
	NSS_BUILTINS_FIRMWARE_VERSION_MINOR };

const NSSUTF8 
nss_builtins_ManufacturerID[] = { "Mozilla Foundation" };

const NSSUTF8 
nss_builtins_LibraryDescription[] = { "NSS Builtin Object Cryptoki Module" };

const NSSUTF8 
nss_builtins_SlotDescription[] = { "NSS Builtin Objects" };

const NSSUTF8 
nss_builtins_TokenLabel[] = { "Builtin Object Token" };

const NSSUTF8 
nss_builtins_TokenModel[] = { "1" };


const NSSUTF8 
nss_builtins_TokenSerialNumber[] = { "1" };

