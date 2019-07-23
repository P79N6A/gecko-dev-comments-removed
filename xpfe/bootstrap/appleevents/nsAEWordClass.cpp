






































#include "def.h"
#include "wasteutil.h"

#include "AEUtils.h"
#include "AETextClass.h"

#include "AEWordClass.h"

#include "nsGfxUtils.h"






UInt32 AEWordIterator::GetNumItems(const AEDesc* containerToken)
{
	AETokenDesc	container(containerToken);
	WEReference	theWE = container.GetWEReference();

	return theWE ? GetWENumWords(theWE) : 0;
}





AEClassIterator::ItemRef AEWordIterator::GetIndexedItemReference(const AEDesc* containerToken, TAEListIndex itemIndex)
{
	return (ItemRef)itemIndex;
}





TAEListIndex AEWordIterator::GetIndexFromItemID(const AEDesc* containerToken, ItemID itemID)
{
	return (TAEListIndex)itemID;
}





AEClassIterator::ItemID AEWordIterator::GetIndexedItemID(const AEDesc* containerToken, TAEListIndex itemIndex)
{
	return (ItemID)GetIndexedItemReference(containerToken, itemIndex);
}





AEClassIterator::ItemID AEWordIterator::GetIDFromReference(const AEDesc* containerToken, ItemRef itemRef)
{
	return (ItemID)itemRef;
}





AEClassIterator::ItemRef AEWordIterator::GetReferenceFromID(const AEDesc* containerToken, ItemID itemID)
{
	return (ItemRef)itemID;
}





AEClassIterator::ItemID AEWordIterator::GetItemIDFromToken(const AEDesc* token)
{
	AETokenDesc	container(token);
	return (ItemID)container.GetElementNumber();
}





void AEWordIterator::SetItemIDInCoreToken(const AEDesc* containerToken, CoreTokenRecord* tokenRecord, ItemID itemID)
{
	AETokenDesc		tokenDesc(containerToken);
	WindowPtr		wind = tokenDesc.GetWindowPtr();
	WEReference		theWE = tokenDesc.GetWEReference();

	tokenRecord->window = wind;
	tokenRecord->theWE = theWE;
	
	tokenRecord->elementNumber = (TAEListIndex)itemID;
}

#pragma mark -






AEWordClass::AEWordClass()
:	AEGenericClass(cWord, AETextClass::cTEText)
{
}





AEWordClass::~AEWordClass()
{

}

#pragma mark -






void AEWordClass::GetItemFromContainer(		DescType			desiredClass,
										const AEDesc*		containerToken,
										DescType			containerClass, 
										DescType			keyForm,
										const AEDesc*		keyData,
										AEDesc*			resultToken)
{
	AEWordIterator		wordIterator;
	wordIterator.GetItemFromContainer(desiredClass, containerToken, containerClass, keyForm, keyData, resultToken);
}






void AEWordClass::CountObjects(				DescType 		 	desiredType,
										DescType 		 	containerClass,
										const AEDesc *		container,
						   				long *			result)
{
	AEWordIterator		wordIterator;
	long		numItems = wordIterator.GetNumItems(container);
	*result = numItems;
}

#pragma mark -






void AEWordClass::GetDataFromObject(const AEDesc *token, AEDesc *desiredTypes, AEDesc *data)
{
	AETokenDesc		tokenDesc(token);

	DescType			aDescType		= cWord;
	DescType 			propertyCode 		= tokenDesc.GetPropertyCode();
	WindowPtr		window        	 	= tokenDesc.GetWindowPtr();
	WEReference		theWE			= tokenDesc.GetWEReference();
	TAEListIndex		wordIndex			= tokenDesc.GetElementNumber();
	OSErr			err				= noErr;
	long				wordStart, wordEnd;
	
	ThrowErrIfNil(theWE, errAENoSuchObject);
	
	switch (propertyCode)
	{
		case pProperties:
			err = AECreateList(nil, 0, true, data);
			ThrowIfOSErr(err);

			err = AEPutKeyPtr(data, pObjectType, typeType, &aDescType, sizeof(DescType));
			break;
			
		case pBestType:
		case pClass:
		case pDefaultType:
		case pObjectType:
			err = AECreateDesc(typeType, &aDescType, sizeof(DescType), data);
			ThrowIfOSErr(err);
			break;
	
		case pContents:
		case typeNull:
			{
				err = GetWEIndexedWordOffsets(theWE, wordIndex, &wordStart, &wordEnd);
				ThrowIfOSErr(err);
			
				Handle			weHand = WEGetText(theWE);
				StHandleLocker		lockHand(weHand);
				
				err = AECreateDesc(typeChar, *weHand + wordStart, wordEnd - wordStart,data);
				ThrowIfOSErr(err);
			}
			break;
			
		default:
			Inherited::GetDataFromObject(token, desiredTypes, data);
			break;
	}
	
	ThrowIfOSErr(err);
}






void AEWordClass::SetDataForObject(const AEDesc *token, AEDesc *data)
{
	AETokenDesc		tokenDesc(token);

	DescType			aDescType		= cWord;
	DescType 			propertyCode 		= tokenDesc.GetPropertyCode();
	WindowPtr		window        	 	= tokenDesc.GetWindowPtr();
	WEReference		theWE			= tokenDesc.GetWEReference();
	TAEListIndex		wordIndex			= tokenDesc.GetElementNumber();
	OSErr			err				= noErr;
	long				wordStart, wordEnd;
	
	ThrowErrIfNil(theWE, errAENoSuchObject);
	
	switch (propertyCode)
	{
	
		case pContents:
		case typeNull:
			{
				err = GetWEIndexedWordOffsets(theWE, wordIndex, &wordStart, &wordEnd);
				ThrowIfOSErr(err);
			
			}
			break;
	}
	
	ThrowIfOSErr(err);
}

#pragma mark -






Boolean AEWordClass::CanSetProperty(DescType propertyCode)
{
	Boolean result = false;
	
	switch (propertyCode)
	{
		case pBestType:
		case pClass:
		case pDefaultType:
		case pObjectType:
			result = false;
			break;
		
		case typeNull:
			result = true;		
			break;
			
		case pProperties:
		default:
			result = Inherited::CanSetProperty(propertyCode);
			break;
	}
	
	return result;
}






Boolean AEWordClass::CanGetProperty(DescType propertyCode)
{
	Boolean result = false;
	
	switch (propertyCode)
	{
		

		case pBestType:
		case pClass:
		case pDefaultType:
		case pObjectType:
		case pProperties:
			result = true;
			break;
			
		default:
			result = Inherited::CanGetProperty(propertyCode);
			break;
	}
	
	return result;
}

#pragma mark -






void AEWordClass::CreateSelfSpecifier(const AEDesc *token, AEDesc *outSpecifier)
{
	AETokenDesc			tokenDesc(token);

	WEReference			theWE = tokenDesc.GetWEReference();
	TAEListIndex			wordIndex = tokenDesc.GetElementNumber();
	
	AEDesc				selfDesc = { typeNull, nil };
	AEDesc				containerSpecifier = { typeNull, nil };
	OSErr				err = noErr;
	
	GetContainerSpecifier(token, &containerSpecifier);
	
	err = ::AECreateDesc(typeLongInteger, &wordIndex, sizeof(TAEListIndex), &selfDesc);
	ThrowIfOSErr(err);
	
	err = ::CreateObjSpecifier(mClass, &containerSpecifier, formAbsolutePosition, &selfDesc, true, outSpecifier);
	ThrowIfOSErr(err);
}
