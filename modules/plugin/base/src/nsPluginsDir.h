




































#ifndef nsPluginsDir_h_
#define nsPluginsDir_h_

#include "nsError.h"
#include "nsIFile.h"






class nsPluginsDir {
public:
	


	static PRBool IsPluginFile(nsIFile* file);
};

struct PRLibrary;

struct nsPluginInfo {
	char* fName;				
	char* fDescription;			
	PRUint32 fVariantCount;
	char** fMimeTypeArray;
	char** fMimeDescriptionArray;
	char** fExtensionArray;
	char* fFileName;
	char* fFullPath;
	char* fVersion;
#ifdef XP_MACOSX
  PRBool fBundle;
#endif
};







class nsPluginFile {
  PRLibrary* pLibrary;
  nsCOMPtr<nsIFile> mPlugin;
public:
	




	nsPluginFile(nsIFile* spec);
	virtual ~nsPluginFile();

	



	nsresult LoadPlugin(PRLibrary* &outLibrary);

	


	nsresult GetPluginInfo(nsPluginInfo &outPluginInfo);

  


	nsresult FreePluginInfo(nsPluginInfo &PluginInfo);

	
	short OpenPluginResource(void);
};

#endif 
