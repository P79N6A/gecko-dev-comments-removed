






































#ifndef __AEGENERICCLASS__
#define __AEGENERICCLASS__


#include "nsAEClassIterator.h"



class AEGenericClass
{
	friend class AEDispatchHandler;
public:
	
						AEGenericClass(DescType classType, DescType containerClass);			
	virtual				~AEGenericClass();


	
	
	
	void					DispatchEvent(					AEDesc*			token, 
													const AppleEvent*	appleEvent,
													AppleEvent*		reply);	

	
	
	
	static pascal OSErr		ItemFromContainerAccessor(		DescType			desiredClass,
													const AEDesc*		containerToken,
													DescType			containerClass, 
													DescType			keyForm,
													const AEDesc*		keyData,
													AEDesc*			resultToken,
													long 				refCon);

protected:	
	
	
	enum {
		kAEExtract			= 'Extr',
		kExtractKeyDestFolder	= 'Fold',
		
		kAESendMessage		= 'Post'
	};
	
	
	virtual void			GetPropertyFromObject(			DescType			desiredClass,
													const AEDesc*		containerToken,
													DescType			containerClass,
													DescType			keyForm,
													const AEDesc*		keyData,
													AEDesc*			resultToken);

	virtual void			GetProperty(					DescType			desiredClass,
													const AEDesc*		containerToken,
													DescType			containerClass,
													DescType			keyForm,
													const AEDesc*		keyData,
													AEDesc*			resultToken);
	
	
	
	
	
	
	
	virtual void			GetItemFromContainer(			DescType			desiredClass,
													const AEDesc*		containerToken,
													DescType			containerClass, 
													DescType			keyForm,
													const AEDesc*		keyData,
													AEDesc*			resultToken) = 0;
										
	
	
	
	virtual void 			CompareObjects(				DescType			comparisonOperator,
													const AEDesc *		object,
													const AEDesc *		descriptorOrObject,
													Boolean *			result);

	virtual void 			CountObjects(					DescType 		 	desiredType,
													DescType 		 	containerClass,
													const AEDesc *		container,
									   				long *			result);

	
	
	
	virtual void			HandleClose(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleCount(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleSetData(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleGetData(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleDataSize(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleDelete(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleDuplicate(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleExists(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleMake(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleMove(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleOpen(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleReOpen(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleRun(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandlePrint(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleQuit(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleSave(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);

	
	
	
	virtual void			HandleExtract(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleSendMessage(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	
	
	
	
	
	virtual void			GetDataFromObject(const AEDesc *token, AEDesc *desiredTypes, AEDesc *data) = 0;
	virtual void			SetDataForObject(const AEDesc *token, AEDesc *data) = 0;

	
	
	

	void					CreateSpecifier(const AEDesc *token, AEDesc *outSpecifier);
	virtual void			GetContainerSpecifier(const AEDesc *token, AEDesc *outContainerSpecifier);

	virtual void			CreateSelfSpecifier(const AEDesc *token, AEDesc *outSpecifier) = 0;

	void					GetDataFromListOrObject(const AEDesc *tokenOrTokenList, AEDesc *desiredTypes, AEDesc *data);
	void					SetDataForListOrObject(const AEDesc *tokenOrTokenList, const AppleEvent *appleEvent, AppleEvent *reply);

	void					GetDataFromList(const AEDesc *srcList, AEDesc *desiredTypes, AEDesc *dstList);
	void 					SetDataForList(const AEDesc *token, AEDesc *data);

	void					GetPropertyFromListOrObject(		DescType			desiredClass,
													const AEDesc*		containerToken,
													DescType			containerClass,
													DescType			keyForm,
													const AEDesc*		keyData,
													AEDesc*			resultToken);
	
	void					GetPropertyFromList(			DescType			desiredClass,
													const AEDesc*		containerToken,
													DescType			containerClass,
													DescType			keyForm,
													const AEDesc*		keyData,
													AEDesc*			resultToken);

	virtual DescType		GetKeyEventDataAs(DescType propertyCode);
	
	virtual Boolean			CanSetProperty(DescType propertyCode) = 0;
	virtual Boolean			CanGetProperty(DescType propertyCode) = 0;

	DescType				GetClass()				{ return mClass; }


	
	
	

	virtual void			MakeNewObject(				const DescType		insertionPosition,
													const AEDesc*		token,
													const AEDesc*		ptrToWithData, 
													const AEDesc*		ptrToWithProperties,
													AppleEvent*		reply);


protected:

	DescType				mClass;
	DescType				mContainerClass;
	
	OSLAccessorUPP		mItemFromContainerAccessor;

};


#endif

