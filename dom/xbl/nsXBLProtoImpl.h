




#ifndef nsXBLProtoImpl_h__
#define nsXBLProtoImpl_h__

#include "nsMemory.h"
#include "nsXBLPrototypeHandler.h"
#include "nsXBLProtoImplMember.h"
#include "nsXBLProtoImplField.h"
#include "nsXBLBinding.h"

class nsXBLPrototypeBinding;
class nsXBLProtoImplAnonymousMethod;

class nsXBLProtoImpl MOZ_FINAL
{
public:
  nsXBLProtoImpl()
    : mPrecompiledMemberHolder(nullptr),
      mMembers(nullptr),
      mFields(nullptr),
      mConstructor(nullptr),
      mDestructor(nullptr)
  {
    MOZ_COUNT_CTOR(nsXBLProtoImpl);
  }
  ~nsXBLProtoImpl()
  {
    MOZ_COUNT_DTOR(nsXBLProtoImpl);
    
    
    delete mMembers;
    delete mFields;
  }


  nsresult InstallImplementation(nsXBLPrototypeBinding* aPrototypeBinding, nsXBLBinding* aBinding);

private:
  nsresult InitTargetObjects(nsXBLPrototypeBinding* aBinding,
                             nsIContent* aBoundElement,
                             JS::MutableHandle<JSObject*> aTargetClassObject,
                             bool* aTargetIsNew);

public:
  nsresult CompilePrototypeMembers(nsXBLPrototypeBinding* aBinding);

  bool LookupMember(JSContext* aCx, nsString& aName, JS::Handle<jsid> aNameAsId,
                    JS::MutableHandle<JSPropertyDescriptor> aDesc,
                    JS::Handle<JSObject*> aClassObject);

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

  void Trace(const TraceCallbacks& aCallbacks, void *aClosure);
  void UnlinkJSObjects();

  nsXBLProtoImplField* FindField(const nsString& aFieldName) const;

  
  
  bool ResolveAllFields(JSContext *cx, JS::Handle<JSObject*> obj) const;

  
  
  void UndefineFields(JSContext* cx, JS::Handle<JSObject*> obj) const;

  bool CompiledMembers() const {
    return mPrecompiledMemberHolder != nullptr;
  }

  nsresult Read(nsIObjectInputStream* aStream,
                nsXBLPrototypeBinding* aBinding);
  nsresult Write(nsIObjectOutputStream* aStream,
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
  JSObject* mPrecompiledMemberHolder; 
                          

  nsXBLProtoImplMember* mMembers; 

  nsXBLProtoImplField* mFields; 

public:
  nsXBLProtoImplAnonymousMethod* mConstructor; 
  nsXBLProtoImplAnonymousMethod* mDestructor;  
};

nsresult
NS_NewXBLProtoImpl(nsXBLPrototypeBinding* aBinding,
                   const char16_t* aClassName,
                   nsXBLProtoImpl** aResult);

#endif 
