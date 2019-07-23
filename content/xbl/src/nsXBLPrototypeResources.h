





































#ifndef nsXBLPrototypeResources_h__
#define nsXBLPrototypeResources_h__

#include "nsCOMPtr.h"
#include "nsICSSLoaderObserver.h"
#include "nsISupportsArray.h"
#include "nsIStyleRuleProcessor.h"
#include "nsCOMArray.h"

class nsIContent;
class nsIAtom;
class nsIDocument;
class nsIScriptContext;
class nsSupportsHashtable;
class nsXBLResourceLoader;
class nsXBLPrototypeBinding;
class nsICSSStyleSheet;




class nsXBLPrototypeResources
{
public:
  void LoadResources(PRBool* aResult);
  void AddResource(nsIAtom* aResourceType, const nsAString& aSrc);
  void AddResourceListener(nsIContent* aElement);
  nsresult FlushSkinSheets();

  nsXBLPrototypeResources(nsXBLPrototypeBinding* aBinding);
  ~nsXBLPrototypeResources();


  nsXBLResourceLoader* mLoader; 
  nsCOMArray<nsICSSStyleSheet> mStyleSheetList; 

  
  nsCOMPtr<nsIStyleRuleProcessor> mRuleProcessor;
};

#endif

