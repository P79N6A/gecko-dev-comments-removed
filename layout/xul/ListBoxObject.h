





#ifndef mozilla_dom_ListBoxObject_h
#define mozilla_dom_ListBoxObject_h

#include "mozilla/dom/BoxObject.h"
#include "nsPIListBoxObject.h"

namespace mozilla {
namespace dom {

class ListBoxObject final : public BoxObject,
                                public nsPIListBoxObject
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSILISTBOXOBJECT

  ListBoxObject();

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  
  virtual nsListBoxBodyFrame* GetListBoxBody(bool aFlush) override;

  
  virtual void Clear() override;
  virtual void ClearCachedValues() override;

  
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
