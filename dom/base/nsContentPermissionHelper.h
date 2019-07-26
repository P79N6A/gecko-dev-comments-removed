



#ifndef nsContentPermissionHelper_h
#define nsContentPermissionHelper_h

#include "nsIContentPermissionPrompt.h"
#include "nsTArray.h"
#include "nsIMutableArray.h"

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

  ContentPermissionType(const nsACString& aType, const nsACString& aAccess);
  virtual ~ContentPermissionType();

protected:
  nsCString mType;
  nsCString mAccess;
};

uint32_t ConvertPermissionRequestToArray(nsTArray<PermissionRequest>& aSrcArray,
                                         nsIMutableArray* aDesArray);

nsresult CreatePermissionArray(const nsACString& aType,
                               const nsACString& aAccess,
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
  virtual ~nsContentPermissionRequestProxy();

  nsresult Init(const nsTArray<mozilla::dom::PermissionRequest>& requests,
                mozilla::dom::ContentPermissionRequestParent* parent);
  void OnParentDestroyed();

 private:
  
  mozilla::dom::ContentPermissionRequestParent* mParent;
  nsTArray<mozilla::dom::PermissionRequest> mPermissionRequests;
};

#endif 
