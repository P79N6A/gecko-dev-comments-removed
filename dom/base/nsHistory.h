




#ifndef nsHistory_h___
#define nsHistory_h___

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMHistory.h"
#include "nsPIDOMWindow.h" 
#include "nsStringFwd.h"
#include "nsWrapperCache.h"

class nsIDocShell;
class nsISHistory;
class nsIWeakReference;
class nsPIDOMWindow;


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

protected:
  nsIDocShell* GetDocShell() const;

  void PushOrReplaceState(JSContext* aCx, JS::Handle<JS::Value> aData,
                          const nsAString& aTitle, const nsAString& aUrl,
                          mozilla::ErrorResult& aRv, bool aReplace);

  already_AddRefed<nsISHistory> GetSessionHistory() const;

  nsCOMPtr<nsIWeakReference> mInnerWindow;
};

#endif 
