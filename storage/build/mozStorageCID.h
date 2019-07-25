





































#ifndef MOZSTORAGECID_H
#define MOZSTORAGECID_H

#define MOZ_STORAGE_CONTRACTID_PREFIX "@mozilla.org/storage"



#define MOZ_STORAGE_CONNECTION_CID \
{ 0xb71a1f84, 0x3a70, 0x4d37, {0xa3, 0x48, 0xf1, 0xba, 0x0e, 0x27, 0xee, 0xad} }

#define MOZ_STORAGE_CONNECTION_CONTRACTID MOZ_STORAGE_CONTRACTID_PREFIX "/connection;1"


#define MOZ_STORAGE_SERVICE_CID \
{ 0xbbbb1d61, 0x438f, 0x4436, {0x92, 0xed, 0x83, 0x08, 0xe5, 0x83, 0x0f, 0xb0} }

#define MOZ_STORAGE_SERVICE_CONTRACTID MOZ_STORAGE_CONTRACTID_PREFIX "/service;1"


#define VACUUMMANAGER_CID \
{ 0x3b667ee0, 0xd2da, 0x4ccc, { 0x9c, 0x3d, 0x95, 0xf2, 0xca, 0x6a, 0x8b, 0x4c } }

#define VACUUMMANAGER_CONTRACTID MOZ_STORAGE_CONTRACTID_PREFIX "/vacuum;1"

#endif 
