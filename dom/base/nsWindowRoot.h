





#ifndef nsWindowRoot_h__
#define nsWindowRoot_h__

class nsPIDOMWindow;
class nsIDOMEvent;
class nsIGlobalObject;

#include "mozilla/Attributes.h"
#include "mozilla/EventListenerManager.h"
#include "nsIDOMEventTarget.h"
#include "nsPIWindowRoot.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"

class nsWindowRoot final : public nsPIWindowRoot
{
public:
  explicit nsWindowRoot(nsPIDOMWindow* aWindow);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMEVENTTARGET

  virtual mozilla::EventListenerManager*
    GetExistingListenerManager() const override;
  virtual mozilla::EventListenerManager*
    GetOrCreateListenerManager() override;

  using mozilla::dom::EventTarget::RemoveEventListener;
  virtual void AddEventListener(const nsAString& aType,
                                mozilla::dom::EventListener* aListener,
                                bool aUseCapture,
                                const mozilla::dom::Nullable<bool>& aWantsUntrusted,
                                mozilla::ErrorResult& aRv) override;

  

  virtual nsPIDOMWindow* GetWindow() override;

  virtual nsresult GetControllers(nsIControllers** aResult) override;
  virtual nsresult GetControllerForCommand(const char * aCommand,
                                           nsIController** _retval) override;

  virtual void GetEnabledDisabledCommands(nsTArray<nsCString>& aEnabledCommands,
                                          nsTArray<nsCString>& aDisabledCommands) override;

  virtual nsIDOMNode* GetPopupNode() override;
  virtual void SetPopupNode(nsIDOMNode* aNode) override;

  virtual void SetParentTarget(mozilla::dom::EventTarget* aTarget) override
  {
    mParent = aTarget;
  }
  virtual mozilla::dom::EventTarget* GetParentTarget() override { return mParent; }
  virtual nsIDOMWindow* GetOwnerGlobal() override;

  nsIGlobalObject* GetParentObject();

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsWindowRoot,
                                                         nsIDOMEventTarget)

protected:
  virtual ~nsWindowRoot();

  void GetEnabledDisabledCommandsForControllers(nsIControllers* aControllers,
                                                nsTHashtable<nsCharPtrHashKey>& aCommandsHandled,
                                                nsTArray<nsCString>& aEnabledCommands,
                                                nsTArray<nsCString>& aDisabledCommands);

  
  nsCOMPtr<nsPIDOMWindow> mWindow;
  
  nsRefPtr<mozilla::EventListenerManager> mListenerManager; 
  nsCOMPtr<nsIDOMNode> mPopupNode; 

  nsCOMPtr<mozilla::dom::EventTarget> mParent;
};

extern already_AddRefed<mozilla::dom::EventTarget>
NS_NewWindowRoot(nsPIDOMWindow* aWindow);

#endif
