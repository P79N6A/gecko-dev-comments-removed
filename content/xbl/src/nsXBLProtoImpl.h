





































#ifndef nsXBLProtoImpl_h__
#define nsXBLProtoImpl_h__

#include "nsMemory.h"
#include "nsXBLPrototypeHandler.h"
#include "nsXBLProtoImplMember.h"
#include "nsXBLPrototypeBinding.h"

class nsIXPConnectJSObjectHolder;

class nsXBLProtoImpl
{
public:
  nsXBLProtoImpl() 
    : mClassObject(nsnull),
      mMembers(nsnull),
      mConstructor(nsnull),
      mDestructor(nsnull)
  { 
    MOZ_COUNT_CTOR(nsXBLProtoImpl); 
  }
  ~nsXBLProtoImpl() 
  { 
    MOZ_COUNT_DTOR(nsXBLProtoImpl);
    
    
    for (nsXBLProtoImplMember* curr = mMembers; curr; curr=curr->GetNext())
      curr->Destroy(mClassObject != nsnull);
    delete mMembers; 
  }
  
  nsresult InstallImplementation(nsXBLPrototypeBinding* aBinding, nsIContent* aBoundElement);
  nsresult InitTargetObjects(nsXBLPrototypeBinding* aBinding, nsIScriptContext* aContext, 
                             nsIContent* aBoundElement, 
                             nsIXPConnectJSObjectHolder** aScriptObjectHolder,
                             void** aTargetClassObject);
  nsresult CompilePrototypeMembers(nsXBLPrototypeBinding* aBinding);

  void SetMemberList(nsXBLProtoImplMember* aMemberList) { delete mMembers; mMembers = aMemberList; }

  void Traverse(nsCycleCollectionTraversalCallback &cb) const;

protected:
  
  
  
  
  void DestroyMembers(nsXBLProtoImplMember* aBrokenMember);
  
public:
  nsCString mClassName; 

protected:
  void* mClassObject;   
                        

  nsXBLProtoImplMember* mMembers; 
  
public:
  nsXBLProtoImplAnonymousMethod* mConstructor; 
  nsXBLProtoImplAnonymousMethod* mDestructor;  
};

nsresult
NS_NewXBLProtoImpl(nsXBLPrototypeBinding* aBinding, 
                   const PRUnichar* aClassName, 
                   nsXBLProtoImpl** aResult);

#endif 
