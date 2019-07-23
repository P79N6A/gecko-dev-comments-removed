





































#ifndef __AEAPPLICATIONCLASS__
#define __AEAPPLICATIONCLASS__

#include "nsAEGenericClass.h"
#include "nsAEDocumentClass.h"		

class AEApplicationClass : public AEGenericClass
{
	friend class AECoreClass;
	friend class AEDocumentClass;

private:
	typedef AEGenericClass	Inherited;
	
protected:
	
						AEApplicationClass();
						~AEApplicationClass();

protected:

	
	virtual void			GetProperty(				DescType			desiredClass,
												const AEDesc*		containerToken,
												DescType			containerClass,
												DescType			keyForm,
												const AEDesc*		keyData,
												AEDesc*			resultToken);

	virtual void			GetItemFromContainer(		DescType			desiredClass,
												const AEDesc*		containerToken,
												DescType			containerClass, 
												DescType			keyForm,
												const AEDesc*		keyData,
												AEDesc*			resultToken);
			
	void 					CountObjects(				DescType 		 	desiredType,
												DescType 		 	containerClass,
												const AEDesc *		container,
								   				long *			result);

protected:

	
	
	
	virtual void			HandleClose(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleCount(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	
	virtual void			HandleDataSize(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleDelete(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleDuplicate(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleExists(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleMake(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleMove(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleRun(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleReOpen(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleOpen(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandlePrint(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleQuit(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleSave(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	
	virtual void			CreateSelfSpecifier(const AEDesc *token, AEDesc *outSpecifier);

	virtual void			GetDataFromObject(const AEDesc *token, AEDesc *desiredTypes, AEDesc *data);
	virtual void			SetDataForObject(const AEDesc *token, AEDesc *data);
	
	virtual Boolean			CanSetProperty(DescType propertyCode);
	virtual Boolean			CanGetProperty(DescType propertyCode);

	virtual DescType		GetKeyEventDataAs(DescType propertyCode);

protected:
	long					CountApplicationObjects(const AEDesc *token, DescType desiredType);
	
};




#endif 
