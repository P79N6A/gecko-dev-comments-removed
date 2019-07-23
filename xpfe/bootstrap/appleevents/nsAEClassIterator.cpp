






































#include "nsAEUtils.h"
#include "nsAETokens.h"

#include "nsAEClassIterator.h"


const AEClassIterator::ItemRef		AEClassIterator::kNoItemRef = -1;
const AEClassIterator::ItemID		AEClassIterator::kNoItemID = -1;





void AEClassIterator::GetItemFromContainer(		DescType			desiredClass,
										const AEDesc*		containerToken,
										DescType			containerClass, 
										DescType			keyForm,
										const AEDesc*		keyData,
										AEDesc*			resultToken)
{
	OSErr				err	= noErr;
	
	CStr255				itemName;
	ItemRef				itemRef = kNoItemRef;
	ItemID				itemID = kNoItemID;
	DescType				keyDataType 	= keyData->descriptorType;

	Boolean				wantsAllItems	= false;

	StAEDesc 				startObject;	
	StAEDesc 				stopObject;
		
	CoreTokenRecord		token;
	
	long					numItems = GetNumItems(containerToken);
	TAEListIndex			itemIndex;
	
	CheckKeyFormSupport(keyForm);		
	
	switch (keyForm)
	{
		case formName:																
			if (DescToCString(keyData, itemName, 255) != noErr)
				ThrowIfOSErr(errAECoercionFail);

			itemRef = GetNamedItemReference(containerToken, itemName);
			if (itemRef == kNoItemRef)
				ThrowIfOSErr(errAENoSuchObject);
			break;
		
		case formAbsolutePosition:														
			itemIndex = NormalizeAbsoluteIndex(keyData, numItems, &wantsAllItems);
			if (wantsAllItems == false)
			{
				if (itemIndex == 0 || itemIndex > numItems)
					ThrowOSErr(errAEIllegalIndex);
			}
			itemRef = GetIndexedItemReference(containerToken, itemIndex);
			if (itemRef == kNoItemRef)
				ThrowOSErr(errAENoSuchObject);
			break;	
		
		case formRelativePosition:
			itemRef = ProcessFormRelativePostition(containerToken, keyData);
			break;	
		
		case formRange:
			switch (keyDataType)
			{		
				case typeRangeDescriptor:
					{
						ProcessFormRange((AEDesc *)keyData, &startObject, &stopObject);
						
						ConstAETokenDesc	startToken(&startObject);
						ConstAETokenDesc	stopToken(&stopObject);
						DescType 		startType = startToken.GetDispatchClass();
						DescType 		stopType  = stopToken.GetDispatchClass();
	 
						if (startType != mClass || stopType != mClass)
							ThrowOSErr(errAEWrongDataType);
					}
					break;

				default:
					ThrowOSErr(errAEWrongDataType);
					break;	
			}
			break;	
		
		default:
			ThrowIfOSErr(errAEEventNotHandled);
	}
	
	
	

	if (wantsAllItems && (err == errAENoSuchObject || err == errAEIllegalIndex))
	{
		err = AECreateList(nil, 0, false, (AEDescList*)resultToken);
		ThrowIfOSErr(err);
		return;
	}

	ThrowIfOSErr(err);

	
	token.dispatchClass 	= GetClass();
	token.objectClass	= GetClass();
	token.propertyCode 	= typeNull;
	
	if (wantsAllItems)
	{
		err = AECreateList(NULL, 0, false, (AEDescList*)resultToken);
		ThrowIfOSErr(err);
		
		for (TAEListIndex index = 1; index <= numItems; index++)
		{
			ItemID		itemID = GetIndexedItemID(containerToken, index);
			if (itemID != kNoItemID)
			{
				SetItemIDInCoreToken(containerToken, &token, itemID);
				err = AEPutPtr(resultToken, 0, desiredClass, &token, sizeof(token));
				ThrowIfOSErr(err);
			}
		}
	}
	else if (keyForm == formRange)
	{
		ConstAETokenDesc		startToken(&startObject);
		ConstAETokenDesc		stopToken(&stopObject);
		
		ItemID			beginItemID 	= GetItemIDFromToken(&startObject);
		ItemID			endItemID	 	= GetItemIDFromToken(&stopObject);
		
		TAEListIndex		beginIndex		= GetIndexFromItemID(containerToken, beginItemID);
		TAEListIndex		endIndex		= GetIndexFromItemID(containerToken, endItemID);
										
		err = AECreateList(nil, 0, false, (AEDescList*)resultToken);
		ThrowIfOSErr(err);
			
		if (beginIndex > endIndex) 
		{
			TAEListIndex	temp = beginIndex;
			beginIndex		= endIndex;
			endIndex   	= temp;
		}
		
		for (TAEListIndex i = beginIndex; i <= endIndex; i ++)
		{
			ItemID		itemID = GetIndexedItemID(containerToken, i);
			if (itemID != kNoItemID)
			{
				SetItemIDInCoreToken(containerToken, &token, itemID);
				err = AEPutPtr(resultToken, 0, desiredClass, &token, sizeof(token));
				ThrowIfOSErr(err);
			}
		}
	}
	else
	{
		SetItemIDInCoreToken(containerToken, &token, GetIDFromReference(containerToken, itemRef));
		err = AECreateDesc(desiredClass, &token, sizeof(token), resultToken);
		ThrowIfOSErr(err);
	}
}







AEClassIterator::ItemRef AEClassIterator::ProcessFormRelativePostition(const AEDesc* anchorToken, const AEDesc *keyData)
{
	OSErr		err = noErr;
	ItemID		anchorItemID = GetItemIDFromToken(anchorToken);
	TAEListIndex	anchorListIndex = GetIndexFromItemID(anchorToken, anchorItemID);
	TAEListIndex	wantedListIndex = 0;
	long			numItems = GetNumItems(anchorToken);
	ItemRef		returnRef = kNoItemRef;

	if (anchorListIndex != 0)
	{
		switch (keyData->descriptorType)
		{
		   case typeEnumerated:
			   	DescType		positionEnum;	
				if (DescToDescType((AEDesc*)keyData, &positionEnum) != noErr)
					ThrowIfOSErr(errAECoercionFail);

				switch (positionEnum)
				{
					case kAENext:						
						wantedListIndex = anchorListIndex + 1;
						if (wantedListIndex > numItems)
							err = errAENoSuchObject;
						break;
						
					case kAEPrevious:					
						wantedListIndex = anchorListIndex - 1;
						if (wantedListIndex < 1)
							err = errAENoSuchObject;
						break;
						
					default:
						err = errAEEventNotHandled;
						break;
				}
				ThrowIfOSErr(err);
				break;
				
			default:
				err = errAECoercionFail;
				break;
		}
	}
	
	ThrowIfOSErr(err);
	return GetIndexedItemReference(anchorToken, wantedListIndex);
}








TAEListIndex AEClassIterator::NormalizeAbsoluteIndex(const AEDesc *keyData, TAEListIndex maxIndex, Boolean *isAllItems)
{
	TAEListIndex	index;
	*isAllItems = false;								
	
	
	
	switch (keyData->descriptorType)
	{
		case typeLongInteger:						
			if (DescToLong(keyData, &index) != noErr)
				ThrowOSErr(errAECoercionFail);
				
			if (index < 0)							
				index = maxIndex + index + 1;
			break;
			
	   case typeAbsoluteOrdinal:										
	   		DescType		ordinalDesc;
			if (DescToDescType((AEDesc*)keyData, &ordinalDesc) != noErr)
				ThrowOSErr(errAECoercionFail);
			
			switch (ordinalDesc)
			{
			   	case kAEFirst:
			   		index = 1;
					break;
					
				case kAEMiddle:
					index = (maxIndex >> 1) + (maxIndex % 2);
					break;
						
			   	case kAELast:
			   		index = maxIndex;
					break;
						
			   	case kAEAny:
					index = (TickCount() % maxIndex) + 1;		
					break;
						
			   	case kAEAll:
			   		index = 1;
			   		*isAllItems = true;
					break;
			}
			break;

		default:
			ThrowOSErr(errAEWrongDataType);
	}

	
	if ((index < 1) || (index > maxIndex))
		ThrowOSErr(errAEIllegalIndex);
		
	return index;
}








void AEClassIterator::ProcessFormRange(AEDesc *keyData, AEDesc *start, AEDesc *stop)
{
	OSErr 		err = noErr;
	StAEDesc 		rangeRecord;
	StAEDesc		ospec;
	
	
	
 	err = AECoerceDesc(keyData, typeAERecord, &rangeRecord);
 	ThrowIfOSErr(err);
	 
	
	err = AEGetKeyDesc(&rangeRecord, keyAERangeStart, typeWildCard, &ospec);
 	if (err == noErr && ospec.descriptorType == typeObjectSpecifier)
 		err = AEResolve(&ospec, kAEIDoMinimum, start);
 		
 	ThrowIfOSErr(err);
 		
	ospec.Clear();
		
	
	
	err = AEGetKeyDesc(&rangeRecord, keyAERangeStop, typeWildCard, &ospec);
  	if (err == noErr && ospec.descriptorType == typeObjectSpecifier)
 		err = AEResolve(&ospec, kAEIDoMinimum, stop);

	ThrowIfOSErr(err);
}







void AEClassIterator::CheckKeyFormSupport(DescType keyForm)
{
	UInt16			testMask;
	switch (keyForm)
	{
		case formAbsolutePosition:		testMask = eHasFormAbsolutePosition;		break;
		case formRelativePosition:		testMask = eHasFormRelativePosition;		break;
		case formTest:					testMask = eHasFormTest;				break;
		case formRange:				testMask = eHasFormRange;				break;
		case formPropertyID:			testMask = eHasFormPropertyID;			break;
		case formName:				testMask = eHasFormName;				break;
		default:
			AE_ASSERT(false, "Unknown key form");
	}
	if ((mKeyFormSupport & testMask) == 0)
		ThrowOSErr(errAEBadKeyForm);
}

#pragma mark -






void AENamedClassIterator::GetItemFromContainer(	DescType			desiredClass,
										const AEDesc*		containerToken,
										DescType			containerClass, 
										DescType			keyForm,
										const AEDesc*		keyData,
										AEDesc*			resultToken)
{
	OSErr				err	= noErr;
	
	CStr255				itemName;
	DescType				keyDataType 	= keyData->descriptorType;

	Boolean				wantsAllItems	= false;

	StAEDesc 				startObject;	
	StAEDesc 				stopObject;
		
	CoreTokenRecord		token;
	
	long					numItems = GetNumItems(containerToken);
	
	CheckKeyFormSupport(keyForm);		
	
	switch (keyForm)
	{
		case formName:																
			if (DescToCString(keyData, itemName, 255) != noErr)
				ThrowIfOSErr(errAECoercionFail);

			if (!NamedItemExists(containerToken, itemName))
				ThrowIfOSErr(errAENoSuchObject);
			break;
		











		
		default:
			ThrowIfOSErr(errAEEventNotHandled);
	}
	
	ThrowIfOSErr(err);

	
	token.dispatchClass 	= GetClass();
	token.objectClass	= GetClass();
	token.propertyCode 	= typeNull;
	
	if (wantsAllItems)
	{
		err = AECreateList(NULL, 0, false, (AEDescList*)resultToken);
		ThrowIfOSErr(err);
		
		for (TAEListIndex index = 1; index <= numItems; index++)
		{
			ItemID		itemID = GetIndexedItemID(containerToken, index);
			if (itemID != kNoItemID)
			{
				SetItemIDInCoreToken(containerToken, &token, itemID);
				err = AEPutPtr(resultToken, 0, desiredClass, &token, sizeof(token));
				ThrowIfOSErr(err);
			}
		}
	}
	else
	{
		SetNamedItemIDInCoreToken(containerToken, &token, itemName);
		err = AECreateDesc(desiredClass, &token, sizeof(token), resultToken);
		ThrowIfOSErr(err);
	}
}

