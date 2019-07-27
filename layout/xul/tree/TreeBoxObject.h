




#ifndef mozilla_dom_TreeBoxObject_h
#define mozilla_dom_TreeBoxObject_h

#include "mozilla/dom/BoxObject.h"
#include "nsITreeView.h"
#include "nsITreeBoxObject.h"

class nsTreeBodyFrame;
class nsTreeColumn;
class nsTreeColumns;

namespace mozilla {
namespace dom {

struct TreeCellInfo;
class DOMRect;

class TreeBoxObject final : public BoxObject,
                                public nsITreeBoxObject
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(TreeBoxObject, BoxObject)
  NS_DECL_NSITREEBOXOBJECT

  TreeBoxObject();

  nsTreeBodyFrame* GetTreeBodyFrame(bool aFlushLayout = false);
  nsTreeBodyFrame* GetCachedTreeBodyFrame() { return mTreeBody; }

  
  virtual void Clear() override;
  virtual void ClearCachedValues() override;

  
  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  already_AddRefed<nsTreeColumns> GetColumns();

  already_AddRefed<nsITreeView> GetView();

  void SetView(nsITreeView* arg, ErrorResult& aRv);

  bool Focused();

  already_AddRefed<Element> GetTreeBody();

  int32_t RowHeight();

  int32_t RowWidth();

  int32_t HorizontalPosition();

  already_AddRefed<nsIScriptableRegion> SelectionRegion();

  int32_t GetFirstVisibleRow();

  int32_t GetLastVisibleRow();

  int32_t GetPageLength();

  int32_t GetRowAt(int32_t x, int32_t y);

  void GetCellAt(int32_t x, int32_t y, TreeCellInfo& aRetVal, ErrorResult& aRv);

  already_AddRefed<DOMRect> GetCoordsForCellItem(int32_t row,
                                                 nsTreeColumn& col,
                                                 const nsAString& element,
                                                 ErrorResult& aRv);

  bool IsCellCropped(int32_t row, nsITreeColumn* col, ErrorResult& aRv);

  
  void GetCellAt(JSContext* cx,
                 int32_t x, int32_t y,
                 JS::Handle<JSObject*> rowOut,
                 JS::Handle<JSObject*> colOut,
                 JS::Handle<JSObject*> childEltOut,
                 ErrorResult& aRv);

  void GetCoordsForCellItem(JSContext* cx,
                            int32_t row,
                            nsTreeColumn& col,
                            const nsAString& element,
                            JS::Handle<JSObject*> xOut,
                            JS::Handle<JSObject*> yOut,
                            JS::Handle<JSObject*> widthOut,
                            JS::Handle<JSObject*> heightOut,
                            ErrorResult& aRv);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

protected:
  nsTreeBodyFrame* mTreeBody;
  nsCOMPtr<nsITreeView> mView;

private:
  ~TreeBoxObject();
};

} 
} 

#endif
