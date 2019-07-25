





































#ifndef nsXBLPrototypeResources_h__
#define nsXBLPrototypeResources_h__

#include "nsCOMPtr.h"
#include "nsICSSLoaderObserver.h"
#include "nsIStyleRuleProcessor.h"

class nsIContent;
class nsIAtom;
class nsIDocument;
class nsIScriptContext;
class nsSupportsHashtable;
class nsXBLResourceLoader;
class nsXBLPrototypeBinding;
class nsCSSStyleSheet;




class nsXBLPrototypeResources
{
public:
  void LoadResources(bool* aResult);
  void AddResource(nsIAtom* aResourceType, const nsAString& aSrc);
  void AddResourceListener(nsIContent* aElement);
  nsresult FlushSkinSheets();

  nsXBLPrototypeResources(nsXBLPrototypeBinding* aBinding);
  ~nsXBLPrototypeResources();


  nsXBLResourceLoader* mLoader; 
  typedef nsTArray<nsRefPtr<nsCSSStyleSheet> > sheet_array_type;
  sheet_array_type mStyleSheetList; 

  
  nsCOMPtr<nsIStyleRuleProcessor> mRuleProcessor;
};

#endif

