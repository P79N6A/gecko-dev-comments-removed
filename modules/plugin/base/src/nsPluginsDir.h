




































#ifndef nsPluginsDir_h___
#define nsPluginsDir_h___

#include "nsError.h"
#include "nsIFile.h"






class nsPluginsDir {
public:
	


	static PRBool IsPluginFile(nsIFile* file);
};

struct PRLibrary;

struct nsPluginInfo {
	PRUint32 fPluginInfoSize;	
	char* fName;				
	char* fDescription;			
	PRUint32 fVariantCount;
	char** fMimeTypeArray;
	char** fMimeDescriptionArray;
	char** fExtensionArray;
	char* fFileName;
	char* fFullPath;
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
