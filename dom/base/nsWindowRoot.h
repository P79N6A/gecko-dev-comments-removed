





#ifndef nsWindowRoot_h__
#define nsWindowRoot_h__

class nsPIDOMWindow;
class nsIDOMEventListener;
class nsIDOMEvent;

namespace mozilla {
class EventChainPostVisitor;
class EventChainPreVisitor;
} 

#include "mozilla/Attributes.h"
#include "mozilla/EventListenerManager.h"
#include "nsIDOMEventTarget.h"
#include "nsPIWindowRoot.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"

class nsWindowRoot : public nsPIWindowRoot
{
public:
  explicit nsWindowRoot(nsPIDOMWindow* aWindow);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMEVENTTARGET

  virtual mozilla::EventListenerManager*
    GetExistingListenerManager() const MOZ_OVERRIDE;
  virtual mozilla::EventListenerManager*
    GetOrCreateListenerManager() MOZ_OVERRIDE;

  using mozilla::dom::EventTarget::RemoveEventListener;
  virtual void AddEventListener(const nsAString& aType,
                                mozilla::dom::EventListener* aListener,
                                bool aUseCapture,
                                const mozilla::dom::Nullable<bool>& aWantsUntrusted,
                                mozilla::ErrorResult& aRv) MOZ_OVERRIDE;

  

  virtual nsPIDOMWindow* GetWindow() MOZ_OVERRIDE;

  virtual nsresult GetControllers(nsIControllers** aResult) MOZ_OVERRIDE;
  virtual nsresult GetControllerForCommand(const char * aCommand,
                                           nsIController** _retval) MOZ_OVERRIDE;

  virtual nsIDOMNode* GetPopupNode() MOZ_OVERRIDE;
  virtual void SetPopupNode(nsIDOMNode* aNode) MOZ_OVERRIDE;

  virtual void SetParentTarget(mozilla::dom::EventTarget* aTarget) MOZ_OVERRIDE
  {
    mParent = aTarget;
  }
  virtual mozilla::dom::EventTarget* GetParentTarget() MOZ_OVERRIDE { return mParent; }
  virtual nsIDOMWindow* GetOwnerGlobal() MOZ_OVERRIDE;

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsWindowRoot,
                                                         nsIDOMEventTarget)

protected:
  virtual ~nsWindowRoot();

  
  nsCOMPtr<nsPIDOMWindow> mWindow;
  
  nsRefPtr<mozilla::EventListenerManager> mListenerManager; 
  nsCOMPtr<nsIDOMNode> mPopupNode; 

  nsCOMPtr<mozilla::dom::EventTarget> mParent;
};

extern already_AddRefed<mozilla::dom::EventTarget>
NS_NewWindowRoot(nsPIDOMWindow* aWindow);

#endif
