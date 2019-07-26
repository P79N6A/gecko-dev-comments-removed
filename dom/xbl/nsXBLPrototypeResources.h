




#ifndef nsXBLPrototypeResources_h__
#define nsXBLPrototypeResources_h__

#include "nsAutoPtr.h"
#include "nsICSSLoaderObserver.h"

class nsCSSRuleProcessor;
class nsCSSStyleSheet;
class nsIAtom;
class nsIContent;
class nsXBLPrototypeBinding;
class nsXBLResourceLoader;




class nsXBLPrototypeResources
{
public:
  nsXBLPrototypeResources(nsXBLPrototypeBinding* aBinding);
  ~nsXBLPrototypeResources();

  void LoadResources(bool* aResult);
  void AddResource(nsIAtom* aResourceType, const nsAString& aSrc);
  void AddResourceListener(nsIContent* aElement);
  nsresult FlushSkinSheets();

  nsresult Write(nsIObjectOutputStream* aStream);

  void Traverse(nsCycleCollectionTraversalCallback &cb) const;

  void ClearLoader();

  typedef nsTArray<nsRefPtr<nsCSSStyleSheet> > sheet_array_type;

private:
  
  nsRefPtr<nsXBLResourceLoader> mLoader;

public:
  
  sheet_array_type mStyleSheetList;

  
  nsRefPtr<nsCSSRuleProcessor> mRuleProcessor;
};

#endif

