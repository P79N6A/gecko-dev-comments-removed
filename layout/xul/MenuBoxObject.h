





#ifndef mozilla_dom_MenuBoxObject_h
#define mozilla_dom_MenuBoxObject_h

#include "mozilla/dom/BoxObject.h"

namespace mozilla {
namespace dom {

class KeyboardEvent;

class MenuBoxObject final : public BoxObject
{
public:

  MenuBoxObject();

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  void OpenMenu(bool aOpenFlag);
  already_AddRefed<Element> GetActiveChild();
  void SetActiveChild(Element* arg);
  bool HandleKeyPress(KeyboardEvent& keyEvent);
  bool OpenedWithKey();

private:
  ~MenuBoxObject();
};

} 
} 

#endif 
