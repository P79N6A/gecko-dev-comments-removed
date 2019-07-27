




#ifndef nsPluginsDir_h_
#define nsPluginsDir_h_

#include "nsError.h"
#include "nsIFile.h"






class nsPluginsDir {
public:
	


	static bool IsPluginFile(nsIFile* file);
};

struct PRLibrary;

struct nsPluginInfo {
	char* fName;				
	char* fDescription;			
	uint32_t fVariantCount;
	char** fMimeTypeArray;
	char** fMimeDescriptionArray;
	char** fExtensionArray;
	char* fFileName;
	char* fFullPath;
	char* fVersion;
};







class nsPluginFile {
  PRLibrary* pLibrary;
  nsCOMPtr<nsIFile> mPlugin;
public:
	




	explicit nsPluginFile(nsIFile* spec);
	virtual ~nsPluginFile();

	



	nsresult LoadPlugin(PRLibrary **outLibrary);

	



	nsresult GetPluginInfo(nsPluginInfo &outPluginInfo, PRLibrary **outLibrary);

  


	nsresult FreePluginInfo(nsPluginInfo &PluginInfo);
};

#endif 
