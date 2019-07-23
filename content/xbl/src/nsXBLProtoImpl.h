





































#ifndef nsXBLProtoImpl_h__
#define nsXBLProtoImpl_h__

#include "nsMemory.h"
#include "nsXBLPrototypeHandler.h"
#include "nsXBLProtoImplMember.h"
#include "nsXBLProtoImplField.h"

class nsIXPConnectJSObjectHolder;
class nsXBLPrototypeBinding;
class nsXBLProtoImplAnonymousMethod;

class nsXBLProtoImpl
{
public:
  nsXBLProtoImpl() 
    : mClassObject(nsnull),
      mMembers(nsnull),
      mFields(nsnull),
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
    delete mFields;
  }
  
  nsresult InstallImplementation(nsXBLPrototypeBinding* aBinding, nsIContent* aBoundElement);
  nsresult InitTargetObjects(nsXBLPrototypeBinding* aBinding, nsIScriptContext* aContext, 
                             nsIContent* aBoundElement, 
                             nsIXPConnectJSObjectHolder** aScriptObjectHolder,
                             void** aTargetClassObject);
  nsresult CompilePrototypeMembers(nsXBLPrototypeBinding* aBinding);

  void SetMemberList(nsXBLProtoImplMember* aMemberList)
  {
    delete mMembers;
    mMembers = aMemberList;
  }

  void SetFieldList(nsXBLProtoImplField* aFieldList)
  {
    delete mFields;
    mFields = aFieldList;
  }

  void Traverse(nsCycleCollectionTraversalCallback &cb) const;
  void Unlink();

  nsXBLProtoImplField* FindField(const nsString& aFieldName) const;

  
  
  PRBool ResolveAllFields(JSContext *cx, JSObject *obj) const;

  PRBool CompiledMembers() const {
    return mClassObject != nsnull;
  }

protected:
  
  
  
  
  void DestroyMembers(nsXBLProtoImplMember* aBrokenMember);
  
public:
  nsCString mClassName; 

protected:
  void* mClassObject;   
                        

  nsXBLProtoImplMember* mMembers; 

  nsXBLProtoImplField* mFields; 
  
public:
  nsXBLProtoImplAnonymousMethod* mConstructor; 
  nsXBLProtoImplAnonymousMethod* mDestructor;  
};

nsresult
NS_NewXBLProtoImpl(nsXBLPrototypeBinding* aBinding, 
                   const PRUnichar* aClassName, 
                   nsXBLProtoImpl** aResult);

#endif 
