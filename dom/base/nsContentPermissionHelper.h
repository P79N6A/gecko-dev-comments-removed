



#ifndef nsContentPermissionHelper_h
#define nsContentPermissionHelper_h

#include "nsIContentPermissionPrompt.h"
#include "nsTArray.h"
#include "nsIMutableArray.h"
#include "mozilla/dom/PContentPermissionRequestChild.h"
#include "mozilla/dom/ipc/IdType.h"
#include "nsIDOMEventListener.h"




#undef LoadImage

class nsPIDOMWindow;
class nsContentPermissionRequestProxy;







namespace IPC {
class Principal;
}

class VisibilityChangeListener final : public nsIDOMEventListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER

  explicit VisibilityChangeListener(nsPIDOMWindow* aWindow);

  void RemoveListener();
  void SetCallback(nsIContentPermissionRequestCallback* aCallback);
  already_AddRefed<nsIContentPermissionRequestCallback> GetCallback();

private:
  virtual ~VisibilityChangeListener() {}

  nsWeakPtr mWindow;
  nsCOMPtr<nsIContentPermissionRequestCallback> mCallback;
};

namespace mozilla {
namespace dom {

class Element;
class PermissionRequest;
class ContentPermissionRequestParent;
class PContentPermissionRequestParent;

class ContentPermissionType : public nsIContentPermissionType
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTENTPERMISSIONTYPE

  ContentPermissionType(const nsACString& aType,
                        const nsACString& aAccess,
                        const nsTArray<nsString>& aOptions);

protected:
  virtual ~ContentPermissionType();

  nsCString mType;
  nsCString mAccess;
  nsTArray<nsString> mOptions;
};

class nsContentPermissionUtils
{
public:
  static uint32_t
  ConvertPermissionRequestToArray(nsTArray<PermissionRequest>& aSrcArray,
                                  nsIMutableArray* aDesArray);

  static uint32_t
  ConvertArrayToPermissionRequest(nsIArray* aSrcArray,
                                  nsTArray<PermissionRequest>& aDesArray);

  static nsresult
  CreatePermissionArray(const nsACString& aType,
                        const nsACString& aAccess,
                        const nsTArray<nsString>& aOptions,
                        nsIArray** aTypesArray);

  static PContentPermissionRequestParent*
  CreateContentPermissionRequestParent(const nsTArray<PermissionRequest>& aRequests,
                                       Element* element,
                                       const IPC::Principal& principal,
                                       const TabId& aTabId);

  static nsresult
  AskPermission(nsIContentPermissionRequest* aRequest, nsPIDOMWindow* aWindow);

  static nsTArray<PContentPermissionRequestParent*>
  GetContentPermissionRequestParentById(const TabId& aTabId);

  static void
  NotifyRemoveContentPermissionRequestParent(PContentPermissionRequestParent* aParent);
};

class nsContentPermissionRequester final : public nsIContentPermissionRequester
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTENTPERMISSIONREQUESTER

  explicit nsContentPermissionRequester(nsPIDOMWindow* aWindow);

private:
  virtual ~nsContentPermissionRequester();

  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsRefPtr<VisibilityChangeListener> mListener;
};

} 
} 

using mozilla::dom::ContentPermissionRequestParent;

class nsContentPermissionRequestProxy : public nsIContentPermissionRequest
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTENTPERMISSIONREQUEST

  nsContentPermissionRequestProxy();

  nsresult Init(const nsTArray<mozilla::dom::PermissionRequest>& requests,
                ContentPermissionRequestParent* parent);

  void OnParentDestroyed();

  void NotifyVisibility(const bool& aIsVisible);

private:
  class nsContentPermissionRequesterProxy final : public nsIContentPermissionRequester {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICONTENTPERMISSIONREQUESTER

    explicit nsContentPermissionRequesterProxy(ContentPermissionRequestParent* aParent)
      : mParent(aParent)
      , mWaitGettingResult(false) {}

    void NotifyVisibilityResult(const bool& aIsVisible);

  private:
    virtual ~nsContentPermissionRequesterProxy() {}

    ContentPermissionRequestParent* mParent;
    bool mWaitGettingResult;
    nsCOMPtr<nsIContentPermissionRequestCallback> mGetCallback;
    nsCOMPtr<nsIContentPermissionRequestCallback> mOnChangeCallback;
  };

  virtual ~nsContentPermissionRequestProxy();

  
  ContentPermissionRequestParent* mParent;
  nsTArray<mozilla::dom::PermissionRequest> mPermissionRequests;
  nsRefPtr<nsContentPermissionRequesterProxy> mRequester;
};




class RemotePermissionRequest final : public nsIContentPermissionRequestCallback
                                    , public mozilla::dom::PContentPermissionRequestChild
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTENTPERMISSIONREQUESTCALLBACK

  RemotePermissionRequest(nsIContentPermissionRequest* aRequest,
                          nsPIDOMWindow* aWindow);

  
  virtual bool Recv__delete__(const bool &aAllow,
                              InfallibleTArray<PermissionChoice>&& aChoices) override;

  virtual bool RecvGetVisibility() override;

  void IPDLAddRef()
  {
    mIPCOpen = true;
    AddRef();
  }

  void IPDLRelease()
  {
    mIPCOpen = false;
    Release();
  }

private:
  virtual ~RemotePermissionRequest()
  {
    MOZ_ASSERT(!mIPCOpen, "Protocol must not be open when RemotePermissionRequest is destroyed.");
  }

  void DoAllow(JS::HandleValue aChoices);
  void DoCancel();

  nsCOMPtr<nsIContentPermissionRequest> mRequest;
  nsCOMPtr<nsPIDOMWindow>               mWindow;
  bool                                  mIPCOpen;
  nsRefPtr<VisibilityChangeListener>    mListener;
};

#endif 

