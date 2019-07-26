




#ifndef nsHistory_h___
#define nsHistory_h___

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/BindingUtils.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "nsIDOMHistory.h"
#include "nsString.h"
#include "nsISHistory.h"
#include "nsIWeakReference.h"
#include "nsPIDOMWindow.h"

class nsIDocShell;


class nsHistory MOZ_FINAL : public nsIDOMHistory, 
                                                  
                            public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsHistory)

public:
  nsHistory(nsPIDOMWindow* aInnerWindow);
  virtual ~nsHistory();

  nsPIDOMWindow* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  uint32_t GetLength(mozilla::ErrorResult& aRv) const;
  JS::Value GetState(JSContext* aCx, mozilla::ErrorResult& aRv) const;
  void Go(int32_t aDelta, mozilla::ErrorResult& aRv);
  void Back(mozilla::ErrorResult& aRv);
  void Forward(mozilla::ErrorResult& aRv);
  void PushState(JSContext* aCx, JS::Handle<JS::Value> aData,
                 const nsAString& aTitle, const nsAString& aUrl,
                 mozilla::ErrorResult& aRv);
  void ReplaceState(JSContext* aCx, JS::Handle<JS::Value> aData,
                    const nsAString& aTitle, const nsAString& aUrl,
                    mozilla::ErrorResult& aRv);
  void GetCurrent(nsString& aRetval, mozilla::ErrorResult& aRv) const;
  void GetPrevious(nsString& aRetval, mozilla::ErrorResult& aRv) const;
  void GetNext(nsString& aRetval, mozilla::ErrorResult& aRv) const;
  void Item(uint32_t aIndex, nsString& aRetval, mozilla::ErrorResult& aRv);
  void IndexedGetter(uint32_t aIndex, bool &aFound, nsString& aRetval,
                     mozilla::ErrorResult& aRv);
  uint32_t Length();

protected:
  nsIDocShell *GetDocShell() const {
    nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
    if (!win)
      return nullptr;
    return win->GetDocShell();
  }

  void PushOrReplaceState(JSContext* aCx, JS::Value aData,
                          const nsAString& aTitle, const nsAString& aUrl,
                          mozilla::ErrorResult& aRv, bool aReplace);

  already_AddRefed<nsISHistory> GetSessionHistory() const;

  nsCOMPtr<nsIWeakReference> mInnerWindow;
};

#endif 
