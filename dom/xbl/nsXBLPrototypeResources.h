




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

  void AppendStyleSheet(nsCSSStyleSheet* aSheet);
  void RemoveStyleSheet(nsCSSStyleSheet* aSheet);
  void InsertStyleSheetAt(size_t aIndex, nsCSSStyleSheet* aSheet);
  nsCSSStyleSheet* StyleSheetAt(size_t aIndex) const;
  size_t SheetCount() const;
  bool HasStyleSheets() const;
  void AppendStyleSheetsTo(nsTArray<nsCSSStyleSheet*>& aResult) const;

  




  void GatherRuleProcessor();

  nsCSSRuleProcessor* GetRuleProcessor() const { return mRuleProcessor; }

private:
  
  nsRefPtr<nsXBLResourceLoader> mLoader;

  
  nsTArray<nsRefPtr<nsCSSStyleSheet>> mStyleSheetList;

  
  nsRefPtr<nsCSSRuleProcessor> mRuleProcessor;
};

#endif
