






































#ifndef __AEWINDOWCLASS__
#define __AEWINDOWCLASS__



#include "nsAEGenericClass.h"


class AEWindowIterator : public AEClassIterator
{
public:

						AEWindowIterator(DescType classType, TWindowKind windowKind)
						: AEClassIterator(classType)
						,	mWindowKind(windowKind)
						{}
						
	virtual UInt32			GetNumItems(const AEDesc* containerToken);
	virtual ItemRef			GetNamedItemReference(const AEDesc* containerToken, const char *itemName);
	virtual ItemRef			GetIndexedItemReference(const AEDesc* containerToken, TAEListIndex itemIndex);
	
	virtual TAEListIndex		GetIndexFromItemID(const AEDesc* containerToken, ItemID itemID);

	virtual ItemID			GetNamedItemID(const AEDesc* containerToken, const char *itemName);
	virtual ItemID			GetIndexedItemID(const AEDesc* containerToken, TAEListIndex itemIndex);

	
	virtual void			GetIndexedItemName(const AEDesc* containerToken, TAEListIndex itemIndex, char *outName, long maxLen);

	
	virtual ItemID			GetIDFromReference(const AEDesc* containerToken, ItemRef itemRef);
	virtual ItemRef			GetReferenceFromID(const AEDesc* containerToken, ItemID itemID);

	virtual ItemID			GetItemIDFromToken(const AEDesc* token);
	virtual void			SetItemIDInCoreToken(const AEDesc* containerToken, CoreTokenRecord* tokenRecord, ItemID itemID);
	
	TWindowKind			GetThisWindowKind() { return mWindowKind; }

protected:
	
	TWindowKind			mWindowKind;
};



class AEWindowClass : public AEGenericClass
{
	friend class AECoreClass;

private:
	typedef AEGenericClass	Inherited;
	
protected:
	
						AEWindowClass(DescType classType, TWindowKind windowKind);
						~AEWindowClass();


	void 					GetDocumentFromWindow(	DescType			desiredClass,		
											const AEDesc*		containerToken,	
											DescType			containerClass,  	 
											DescType			keyForm,
											const AEDesc*		keyData,
											AEDesc*			resultToken);		

	virtual void			GetItemFromContainer(	DescType			desiredClass,
											const AEDesc*		containerToken,
											DescType			containerClass, 
											DescType			keyForm,
											const AEDesc*		keyData,
											AEDesc*			resultToken);

	virtual void 			CompareObjects(		DescType			comparisonOperator,
											const AEDesc *		object,
											const AEDesc *		descriptorOrObject,
											Boolean *			result);

	virtual void 			CountObjects(			DescType 		 	desiredType,
											DescType 		 	containerClass,
											const AEDesc *		container,
							   				long *			result);

public:
	
	
	static pascal OSErr		DocumentAccessor(		DescType			desiredClass,		
											const AEDesc*		containerToken,	
											DescType			containerClass,  	 
											DescType			keyForm,
											const AEDesc*		keyData,
											AEDesc*			resultToken,		
											long 				refCon);

protected:

	
	
	
	virtual void			HandleClose(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleDataSize(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleDelete(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleDuplicate(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleExists(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleMake(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleMove(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleOpen(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandlePrint(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleQuit(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	virtual void			HandleSave(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply);
	
	
	
	

	virtual void			CreateSelfSpecifier(const AEDesc *token, AEDesc *outSpecifier);

	
	
	

	virtual void			GetDataFromObject(const AEDesc *token, AEDesc *desiredTypes, AEDesc *data);
	virtual void			SetDataForObject(const AEDesc *token, AEDesc *data);

	virtual Boolean			CanSetProperty(DescType propertyCode);
	virtual Boolean			CanGetProperty(DescType propertyCode);

	void					SetWindowProperties(WindowPtr wind, const AEDesc *propertyRecord);
	void					MakeWindowObjectSpecifier(WindowPtr wind, AEDesc *outSpecifier);
	
	WindowPtr			GetWindowByIndex(TWindowKind windowKind, long index);
	WindowPtr			GetWindowByTitle(TWindowKind windowKind, ConstStr63Param title);
	
	long					GetWindowIndex(TWindowKind windowKind, WindowPtr window);
	WindowPtr			GetPreviousWindow(TWindowKind windowKind, WindowPtr wind);

	TWindowKind			GetThisWindowKind()		{ return mWindowKind; }
	
public:

	static long				CountWindows(TWindowKind windowKind);

protected:

	TWindowKind			mWindowKind;

protected:

	OSLAccessorUPP		mDocumentAccessor;
	
};


#endif 


