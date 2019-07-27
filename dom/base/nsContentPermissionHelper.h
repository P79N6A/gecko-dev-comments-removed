



#ifndef nsContentPermissionHelper_h
#define nsContentPermissionHelper_h

#include "nsIContentPermissionPrompt.h"
#include "nsTArray.h"
#include "nsIMutableArray.h"
#include "mozilla/dom/PContentPermissionRequestChild.h"



#undef LoadImage

class nsPIDOMWindow;
class nsContentPermissionRequestProxy;







namespace IPC {
class Principal;
}

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
                                       const IPC::Principal& principal);

  static nsresult
  AskPermission(nsIContentPermissionRequest* aRequest, nsPIDOMWindow* aWindow);
};

} 
} 

class nsContentPermissionRequestProxy : public nsIContentPermissionRequest
{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTENTPERMISSIONREQUEST

  nsContentPermissionRequestProxy();

  nsresult Init(const nsTArray<mozilla::dom::PermissionRequest>& requests,
                mozilla::dom::ContentPermissionRequestParent* parent);

  void OnParentDestroyed();

 private:
  virtual ~nsContentPermissionRequestProxy();

  
  mozilla::dom::ContentPermissionRequestParent* mParent;
  nsTArray<mozilla::dom::PermissionRequest> mPermissionRequests;
};




class RemotePermissionRequest MOZ_FINAL : public nsISupports
                                        , public mozilla::dom::PContentPermissionRequestChild
{
public:
  NS_DECL_ISUPPORTS

  RemotePermissionRequest(nsIContentPermissionRequest* aRequest,
                          nsPIDOMWindow* aWindow);

  
  virtual bool Recv__delete__(const bool &aAllow,
                              const nsTArray<PermissionChoice>& aChoices) MOZ_OVERRIDE;

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
};

#endif 
