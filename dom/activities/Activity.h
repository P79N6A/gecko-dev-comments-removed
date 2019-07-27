



#ifndef mozilla_dom_activities_Activity_h
#define mozilla_dom_activities_Activity_h

#include "DOMRequest.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/MozActivityBinding.h"
#include "nsIActivityProxy.h"
#include "mozilla/Preferences.h"
#include "nsPIDOMWindow.h"

namespace mozilla {
namespace dom {

class Activity : public DOMRequest
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(Activity, DOMRequest)

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  static already_AddRefed<Activity>
  Constructor(const GlobalObject& aOwner,
              const ActivityOptions& aOptions,
              ErrorResult& aRv)
  {
    nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aOwner.GetAsSupports());
    if (!window) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }

    nsRefPtr<Activity> activity = new Activity(window);
    aRv = activity->Initialize(window, aOwner.Context(), aOptions);
    return activity.forget();
  }

  explicit Activity(nsPIDOMWindow* aWindow);

protected:
  nsresult Initialize(nsPIDOMWindow* aWindow,
                      JSContext* aCx,
                      const ActivityOptions& aOptions);

  nsCOMPtr<nsIActivityProxy> mProxy;

  ~Activity();
};

} 
} 

#endif 
