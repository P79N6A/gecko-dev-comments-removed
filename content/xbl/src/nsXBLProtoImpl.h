





































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
    
    
    delete mMembers;
    delete mFields;
  }
  
  nsresult InstallImplementation(nsXBLPrototypeBinding* aBinding, nsIContent* aBoundElement);
  nsresult InitTargetObjects(nsXBLPrototypeBinding* aBinding, nsIScriptContext* aContext, 
                             nsIContent* aBoundElement, 
                             nsIXPConnectJSObjectHolder** aScriptObjectHolder,
                             JSObject** aTargetClassObject);
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

  void Trace(TraceCallback aCallback, void *aClosure) const;
  void UnlinkJSObjects();

  nsXBLProtoImplField* FindField(const nsString& aFieldName) const;

  
  
  bool ResolveAllFields(JSContext *cx, JSObject *obj) const;

  
  
  void UndefineFields(JSContext* cx, JSObject* obj) const;

  bool CompiledMembers() const {
    return mClassObject != nsnull;
  }

  nsresult Read(nsIScriptContext* aContext,
                nsIObjectInputStream* aStream,
                nsXBLPrototypeBinding* aBinding,
                nsIScriptGlobalObject* aGlobal);
  nsresult Write(nsIScriptContext* aContext,
                 nsIObjectOutputStream* aStream,
                 nsXBLPrototypeBinding* aBinding);

protected:
  
  nsXBLProtoImplMember* AddMember(nsXBLProtoImplMember* aMember,
                                  nsXBLProtoImplMember* aPreviousMember)
  {
    if (aPreviousMember)
      aPreviousMember->SetNext(aMember);
    else
      mMembers = aMember;
    return aMember;
  }

  void DestroyMembers();
  
public:
  nsCString mClassName; 

protected:
  JSObject* mClassObject; 
                          

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
