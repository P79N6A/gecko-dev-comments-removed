




#ifndef mozilla_dom_StyleSheetList_h
#define mozilla_dom_StyleSheetList_h

#include "nsIDOMStyleSheetList.h"

class nsCSSStyleSheet;

namespace mozilla {
namespace dom {

class StyleSheetList : public nsIDOMStyleSheetList
{
public:
  static StyleSheetList* FromSupports(nsISupports* aSupports)
  {
    nsIDOMStyleSheetList* list = static_cast<nsIDOMStyleSheetList*>(aSupports);
#ifdef DEBUG
    {
      nsCOMPtr<nsIDOMStyleSheetList> list_qi = do_QueryInterface(aSupports);

      
      
      
      MOZ_ASSERT(list_qi == list, "Uh, fix QI!");
    }
#endif
    return static_cast<StyleSheetList*>(list);
  }

  virtual nsCSSStyleSheet* GetItemAt(uint32_t aIndex) = 0;
};

} 
} 

#endif 
