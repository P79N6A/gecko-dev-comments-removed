





#ifndef mozilla_dom_ListBoxObject_h
#define mozilla_dom_ListBoxObject_h

#include "mozilla/dom/BoxObject.h"
#include "nsPIListBoxObject.h"

namespace mozilla {
namespace dom {

class ListBoxObject MOZ_FINAL : public BoxObject,
                                public nsPIListBoxObject
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSILISTBOXOBJECT

  ListBoxObject();

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) MOZ_OVERRIDE;

  
  virtual nsListBoxBodyFrame* GetListBoxBody(bool aFlush) MOZ_OVERRIDE;

  
  virtual void Clear() MOZ_OVERRIDE;
  virtual void ClearCachedValues() MOZ_OVERRIDE;

  
  int32_t GetRowCount();
  int32_t GetNumberOfVisibleRows();
  int32_t GetIndexOfFirstVisibleRow();
  void EnsureIndexIsVisible(int32_t rowIndex);
  void ScrollToIndex(int32_t rowIndex);
  void ScrollByLines(int32_t numLines);
  already_AddRefed<Element> GetItemAtIndex(int32_t index);
  int32_t GetIndexOfItem(Element& item);

protected:
  nsListBoxBodyFrame *mListBoxBody;

private:
  ~ListBoxObject();
};

} 
} 

#endif 
