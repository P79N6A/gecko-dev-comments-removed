




#ifndef nsXBLPrototypeResources_h__
#define nsXBLPrototypeResources_h__

#include "nsAutoPtr.h"
#include "nsICSSLoaderObserver.h"

class nsCSSRuleProcessor;
class nsIAtom;
class nsIContent;
class nsXBLPrototypeBinding;
class nsXBLResourceLoader;

namespace mozilla {
class CSSStyleSheet;
} 




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

  void Traverse(nsCycleCollectionTraversalCallback &cb);
  void Unlink();

  void ClearLoader();

  void AppendStyleSheet(mozilla::CSSStyleSheet* aSheet);
  void RemoveStyleSheet(mozilla::CSSStyleSheet* aSheet);
  void InsertStyleSheetAt(size_t aIndex, mozilla::CSSStyleSheet* aSheet);
  mozilla::CSSStyleSheet* StyleSheetAt(size_t aIndex) const;
  size_t SheetCount() const;
  bool HasStyleSheets() const;
  void AppendStyleSheetsTo(nsTArray<mozilla::CSSStyleSheet*>& aResult) const;

  




  void GatherRuleProcessor();

  nsCSSRuleProcessor* GetRuleProcessor() const { return mRuleProcessor; }

private:
  
  nsRefPtr<nsXBLResourceLoader> mLoader;

  
  nsTArray<nsRefPtr<mozilla::CSSStyleSheet>> mStyleSheetList;

  
  nsRefPtr<nsCSSRuleProcessor> mRuleProcessor;
};

#endif
