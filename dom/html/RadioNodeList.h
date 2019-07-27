





#ifndef mozilla_dom_RadioNodeList_h
#define mozilla_dom_RadioNodeList_h

#include "nsContentList.h"
#include "nsCOMPtr.h"
#include "HTMLFormElement.h"

#define MOZILLA_DOM_RADIONODELIST_IMPLEMENTATION_IID \
  { 0xbba7f3e8, 0xf3b5, 0x42e5, \
  { 0x82, 0x08, 0xa6, 0x8b, 0xe0, 0xbc, 0x22, 0x19 } }

namespace mozilla {
namespace dom {

class RadioNodeList : public nsSimpleContentList
{
public:
  explicit RadioNodeList(HTMLFormElement* aForm) : nsSimpleContentList(aForm) { }

  virtual JSObject* WrapObject(JSContext *cx, JS::Handle<JSObject*> aGivenProto) MOZ_OVERRIDE;
  void GetValue(nsString& retval);
  void SetValue(const nsAString& value);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_DOM_RADIONODELIST_IMPLEMENTATION_IID)
private:
  ~RadioNodeList() { }
};

NS_DEFINE_STATIC_IID_ACCESSOR(RadioNodeList, MOZILLA_DOM_RADIONODELIST_IMPLEMENTATION_IID)

} 
} 

#endif 
