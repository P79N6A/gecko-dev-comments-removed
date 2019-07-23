




































#ifndef	nsIRDFFileSystem_h__
#define	nsIRDFFileSystem_h__

#include "nscore.h"
#include "nsISupports.h"
#include "nsIRDFNode.h"

class nsVoidArray;


#define NS_IRDFFILESYSTEMDATAOURCE_IID \
{ 0x1222e6f0, 0xa5e3, 0x11d2, { 0x8b, 0x7c, 0x00, 0x80, 0x5f, 0x8a, 0x7d, 0xb5 } }

class nsIRDFFileSystemDataSource : public nsIRDFDataSource
{
public:
};



#define NS_IRDFFILESYSTEM_IID \
{ 0x204a1a00, 0xa5e4, 0x11d2, { 0x8b, 0x7c, 0x00, 0x80, 0x5f, 0x8a, 0x7d, 0xb5 } }

class nsIRDFFileSystem : public nsIRDFResource
{
public:
	NS_IMETHOD	GetFolderList(nsIRDFResource *source, nsVoidArray **array) const = 0;
	NS_IMETHOD	GetName(nsVoidArray **array) const = 0;
	NS_IMETHOD	GetURL(nsVoidArray **array) const = 0;
};



#endif
