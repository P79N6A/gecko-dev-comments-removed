



#ifndef nsContentPermissionHelper_h
#define nsContentPermissionHelper_h

#include "nsIContentPermissionPrompt.h"
#include "nsTArray.h"
#include "nsIMutableArray.h"
#include "PCOMContentPermissionRequestChild.h"

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

uint32_t ConvertPermissionRequestToArray(nsTArray<PermissionRequest>& aSrcArray,
                                         nsIMutableArray* aDesArray);

nsresult CreatePermissionArray(const nsACString& aType,
                               const nsACString& aAccess,
                               const nsTArray<nsString>& aOptions,
                               nsIArray** aTypesArray);

PContentPermissionRequestParent*
CreateContentPermissionRequestParent(const nsTArray<PermissionRequest>& aRequests,
                                     Element* element,
                                     const IPC::Principal& principal);

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




class RemotePermissionRequest : public nsIContentPermissionRequest
                              , public PCOMContentPermissionRequestChild
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTENTPERMISSIONREQUEST

  RemotePermissionRequest(nsIContentPermissionRequest* aRequest,
                          nsPIDOMWindow* aWindow);

  
  virtual bool Recv__delete__(const bool &aAllow,
                              const nsTArray<PermissionChoice>& aChoices) MOZ_OVERRIDE;
  virtual void IPDLRelease() MOZ_OVERRIDE { Release(); }

  static uint32_t ConvertArrayToPermissionRequest(
                                nsIArray* aSrcArray,
                                nsTArray<PermissionRequest>& aDesArray);
private:
  virtual ~RemotePermissionRequest() {}

  nsCOMPtr<nsIContentPermissionRequest> mRequest;
  nsCOMPtr<nsPIDOMWindow>               mWindow;
};

#endif 
