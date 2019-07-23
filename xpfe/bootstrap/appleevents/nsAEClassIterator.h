






































#ifndef __AECLASSITERATOR__
#define __AECLASSITERATOR__


#include "nsAETokens.h"

class AEClassIterator
{

protected:

	enum
	{
		eFormAbsolutePositionBit		= 0,
		eFormRelativePositionBit,
		eFormTestBit,
		eFormRangeBit,
		eFormPropertyIDBit,
		eFormNameBit
	};
	
	
public:

	enum
	{
		eHasFormAbsolutePosition		= (1 << eFormAbsolutePositionBit),
		eHasFormRelativePosition		= (1 << eFormRelativePositionBit),
		eHasFormTest				= (1 << eFormTestBit),
		eHasFormRange				= (1 << eFormPropertyIDBit),
		eHasFormPropertyID			= (1 << eFormRangeBit),
		eHasFormName				= (1 << eFormNameBit),
		
		eStandardKeyFormSupport = (eHasFormAbsolutePosition | eHasFormRelativePosition | eHasFormRange | eHasFormName)
	};
	
	
						AEClassIterator(DescType classType, UInt16 keyFormSupport = eStandardKeyFormSupport)
						:	mClass(classType)
						,	mKeyFormSupport(keyFormSupport)
						{}
						
	virtual 				~AEClassIterator() {}
	
	virtual void			GetItemFromContainer(		DescType			desiredClass,
												const AEDesc*		containerToken,
												DescType			containerClass, 
												DescType			keyForm,
												const AEDesc*		keyData,
												AEDesc*			resultToken);

	virtual UInt32			GetNumItems(const AEDesc* containerToken) = 0;			

	typedef UInt32			ItemRef;				
	typedef UInt32			ItemID;				

protected:
	
	static const ItemRef		kNoItemRef;			
	static const ItemID		kNoItemID;			


	virtual ItemRef			GetNamedItemReference(const AEDesc* containerToken, const char *itemName) = 0;
	virtual ItemRef			GetIndexedItemReference(const AEDesc* containerToken, TAEListIndex itemIndex) = 0;
	
	virtual TAEListIndex		GetIndexFromItemID(const AEDesc* containerToken, ItemID itemID) = 0;
	
	virtual ItemID			GetNamedItemID(const AEDesc* containerToken, const char *itemName) = 0;
	virtual ItemID			GetIndexedItemID(const AEDesc* containerToken, TAEListIndex itemIndex) = 0;

	
	virtual void			GetIndexedItemName(const AEDesc* containerToken, TAEListIndex itemIndex, char *outName, long maxLen) = 0;

	
	virtual ItemID			GetIDFromReference(const AEDesc* containerToken, ItemRef itemRef) = 0;
	virtual ItemRef			GetReferenceFromID(const AEDesc* containerToken, ItemID itemID) = 0;

	virtual ItemID			GetItemIDFromToken(const AEDesc* token) = 0;
	virtual void			SetItemIDInCoreToken(const AEDesc* containerToken, CoreTokenRecord* tokenRecord, ItemID itemID) = 0;
	
	
	virtual ItemRef			ProcessFormRelativePostition(const AEDesc* anchorToken, const AEDesc *keyData);
	TAEListIndex			NormalizeAbsoluteIndex(const AEDesc *keyData, TAEListIndex maxIndex, Boolean *isAllItems);
	void					ProcessFormRange(AEDesc *keyData, AEDesc *start, AEDesc *stop);

	void					CheckKeyFormSupport(DescType keyForm);		
	
	DescType				GetClass() { return mClass; }
	

	DescType				mClass;
	UInt16				mKeyFormSupport;
};


class AENamedClassIterator : public AEClassIterator
{
public:
						AENamedClassIterator(DescType classType)
						: AEClassIterator(classType, eHasFormName)
						{}
						
						~AENamedClassIterator() {}

	virtual void			GetItemFromContainer(		DescType			desiredClass,
												const AEDesc*		containerToken,
												DescType			containerClass, 
												DescType			keyForm,
												const AEDesc*		keyData,
												AEDesc*			resultToken);

protected:

	
	
	virtual UInt32			GetNumItems(const AEDesc* containerToken) { return 0; }

	virtual ItemRef			GetNamedItemReference(const AEDesc* containerToken, const char *itemName) { return kNoItemRef; }
	virtual ItemRef			GetIndexedItemReference(const AEDesc* containerToken, TAEListIndex itemIndex) { return kNoItemRef; }
	
	virtual TAEListIndex		GetIndexFromItemID(const AEDesc* containerToken, ItemID itemID) { return 0; }
	
	virtual Boolean			NamedItemExists(const AEDesc* containerToken, const char *itemName) = 0;

	virtual ItemID			GetNamedItemID(const AEDesc* containerToken, const char *itemName) { return kNoItemID; }
	virtual ItemID			GetIndexedItemID(const AEDesc* containerToken, TAEListIndex itemIndex) { return kNoItemID; }

	
	virtual void			GetIndexedItemName(const AEDesc* containerToken, TAEListIndex itemIndex, char *outName, long maxLen) {}

	
	virtual ItemID			GetIDFromReference(const AEDesc* containerToken, ItemRef itemRef) { return kNoItemID; }
	virtual ItemRef			GetReferenceFromID(const AEDesc* containerToken, ItemID itemID) { return kNoItemRef; }

	virtual ItemID			GetItemIDFromToken(const AEDesc* token) { return kNoItemID; }
	virtual void			SetItemIDInCoreToken(const AEDesc* containerToken, CoreTokenRecord* tokenRecord, ItemID itemID) {}
	virtual void			SetNamedItemIDInCoreToken(const AEDesc* containerToken, CoreTokenRecord* token, const char *itemName) = 0;

};



class AEUnnamedClassIterator : public AEClassIterator
{
public:

	enum
	{
		eNoNameKeyFormSupport = (eHasFormAbsolutePosition | eHasFormRelativePosition | eHasFormRange)
	};
	

						AEUnnamedClassIterator(DescType classType)
						: AEClassIterator(classType, eNoNameKeyFormSupport)
						{}
						
						~AEUnnamedClassIterator() {}

protected:

	virtual ItemRef			GetNamedItemReference(const AEDesc* containerToken, const char *itemName) { ThrowOSErr(errAEEventNotHandled); return 0; }
	virtual ItemID			GetNamedItemID(const AEDesc* containerToken, const char *itemName) { ThrowOSErr(errAEEventNotHandled); return 0; }
	virtual void			GetIndexedItemName(const AEDesc* containerToken, TAEListIndex itemIndex, char *outName, long maxLen) { ThrowOSErr(errAEEventNotHandled); }

};


#endif
