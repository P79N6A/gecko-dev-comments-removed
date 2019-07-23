






































#include <string.h>
#include <AEPackObject.h>

#include "nsMemory.h"
#include "nsWindowUtils.h"

#include "nsAppleEvents.h"

#include "nsAEUtils.h"
#include "nsAETokens.h"
#include "nsAECoreClass.h"
#include "nsAEApplicationClass.h"

#include "nsAEWindowClass.h"

#include "nsGfxUtils.h"

using namespace nsWindowUtils;





UInt32 AEWindowIterator::GetNumItems(const AEDesc* containerToken)
{
	return CountWindowsOfKind(mWindowKind);
}





AEClassIterator::ItemRef AEWindowIterator::GetNamedItemReference(const AEDesc* containerToken, const char *itemName)
{
	WindowPtr	wind;
	wind = GetNamedOrFrontmostWindow(mWindowKind, itemName);
	return (wind) ? (ItemRef)wind : kNoItemRef;
}





AEClassIterator::ItemRef AEWindowIterator::GetIndexedItemReference(const AEDesc* containerToken, TAEListIndex itemIndex)
{
	WindowPtr	wind = GetIndexedWindowOfKind(mWindowKind, itemIndex);
	return (wind) ? (ItemRef)wind : kNoItemRef;
}






AEClassIterator::ItemID AEWindowIterator::GetNamedItemID(const AEDesc* containerToken, const char *itemName)
{
	WindowPtr	wind;
	wind = GetNamedOrFrontmostWindow(mWindowKind, itemName);
	return (wind) ? (ItemRef)wind : kNoItemID;
}





TAEListIndex AEWindowIterator::GetIndexFromItemID(const AEDesc* containerToken, ItemID itemID)
{
	return GetWindowIndex(mWindowKind, (WindowPtr)itemID);
}





AEClassIterator::ItemID AEWindowIterator::GetIndexedItemID(const AEDesc* containerToken, TAEListIndex itemIndex)
{
	WindowPtr	wind = GetIndexedWindowOfKind(mWindowKind, itemIndex);
	return (wind) ? (ItemRef)wind : kNoItemID;
}





void	AEWindowIterator::GetIndexedItemName(const AEDesc* containerToken, TAEListIndex itemIndex, char *outName, long maxLen)
{
	WindowPtr	wind = GetIndexedWindowOfKind(mWindowKind, itemIndex);
	GetCleanedWindowName(wind, outName, maxLen);
}





AEClassIterator::ItemID AEWindowIterator::GetIDFromReference(const AEDesc* containerToken, ItemRef itemRef)
{
	return (ItemID)itemRef;
}





AEClassIterator::ItemRef AEWindowIterator::GetReferenceFromID(const AEDesc* containerToken, ItemID itemID)
{
	return (ItemRef)itemID;
}





AEClassIterator::ItemID AEWindowIterator::GetItemIDFromToken(const AEDesc* token)
{
	ConstAETokenDesc	tokenDesc(token);
	return (ItemID)tokenDesc.GetWindowPtr();
}





void AEWindowIterator::SetItemIDInCoreToken(const AEDesc* containerToken, CoreTokenRecord* tokenRecord, ItemID itemID)
{
	tokenRecord->window = (WindowPtr)itemID;
}


#pragma mark -





AEWindowClass::AEWindowClass(DescType classType, TWindowKind windowKind)
:	AEGenericClass(classType, typeNull)
,	mWindowKind(windowKind)
,	mDocumentAccessor(nil)
{
}





AEWindowClass::~AEWindowClass()
{
	if (mDocumentAccessor)
		DisposeOSLAccessorUPP(mDocumentAccessor);
}

#pragma mark -






void AEWindowClass::GetItemFromContainer(		DescType			desiredClass,
										const AEDesc*		containerToken,
										DescType			containerClass, 
										DescType			keyForm,
										const AEDesc*		keyData,
										AEDesc*			resultToken)
{
	AEWindowIterator		windowIterator(GetClass(), GetThisWindowKind());
	windowIterator.GetItemFromContainer(desiredClass, containerToken, containerClass, keyForm, keyData, resultToken);
}





void AEWindowClass::CompareObjects(				DescType			comparisonOperator,
											const AEDesc *		object1,
											const AEDesc *		object2,
											Boolean *			result)
{
	

}






void AEWindowClass::CountObjects(				DescType 		 	desiredType,
										DescType 		 	containerClass,
										const AEDesc *		container,
						   				long *			result)
{
	*result = CountWindows(mWindowKind);
}

#pragma mark -





void AEWindowClass::HandleClose(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	OSErr 		err 	= noErr;	
	
	StAEDesc		saving;
	StAEDesc		savingIn;
	
	AEDesc		*ptrToSaving	= nil;
	AEDesc		*ptrToSavingIn = nil;
	
	
	err = AEGetParamDesc(appleEvent, keyAESaveOptions, typeWildCard, &saving);
	
	if (err == noErr)
		ptrToSaving = &saving;
	else if (err == errAEDescNotFound)
	   err = noErr;
	else
		ThrowIfOSErr(err);
	
	
	err = AEGetParamDesc(appleEvent, keyAEFile, typeFSS, &savingIn);
	
	if (err == noErr)
		ptrToSavingIn = &savingIn;
	else if (err == errAEDescNotFound)
	   err = noErr;
	else
		ThrowIfOSErr(err);

	
	err = CheckForUnusedParameters(appleEvent);
	ThrowIfOSErr(err);
		
	
	if (AEListUtils::TokenContainsTokenList(token))
	{
		long			numItems;
		long			itemNum;
		AEKeyword	keyword;
				
		err = AECountItems(token, &numItems);
		ThrowIfOSErr(err);
		
		if (numItems > 0)
		{
			for (itemNum = 1; itemNum <= numItems; itemNum++)
			{
				StAEDesc		windowToken;
				err = AEGetNthDesc(token, itemNum, typeWildCard, &keyword, &windowToken);
				if (err != noErr) break;
					
				
			}
		}
	}
	else
	{
		
	}
}





void AEWindowClass::HandleDataSize(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowIfOSErr(errAEEventNotHandled);
}








void AEWindowClass::HandleDelete(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowIfOSErr(errAEEventNotHandled);
}





void AEWindowClass::HandleDuplicate(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowIfOSErr(errAEEventNotHandled);
}





void AEWindowClass::HandleExists(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowIfOSErr(errAEEventNotHandled);
}







void AEWindowClass::HandleMake(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowIfOSErr(errAEEventNotHandled);
}






void AEWindowClass::HandleMove(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowIfOSErr(errAEEventNotHandled);
}






void AEWindowClass::HandleOpen(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowIfOSErr(errAEEventNotHandled);
}






void AEWindowClass::HandlePrint(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowIfOSErr(errAEEventNotHandled);
}






void AEWindowClass::HandleQuit(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowIfOSErr(errAEEventNotHandled);
}






void AEWindowClass::HandleSave(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowIfOSErr(errAEEventNotHandled);
}

#pragma mark -







void AEWindowClass::GetDataFromObject(const AEDesc *token, AEDesc *desiredTypes, AEDesc *data)
{
	OSErr			err				= noErr;
	
	ConstAETokenDesc		tokenDesc(token);
	DescType			aType = mClass;
	Rect				aRect;
	
	Point				aPoint;
	Boolean			aBoolean;
	long				index;
	CStr255			windowTitle;
	char*				urlString = NULL;
	
	DescType 			propertyCode 		= tokenDesc.GetPropertyCode();
	WindowPtr		window        	 	= tokenDesc.GetWindowPtr();
		
	switch (propertyCode)
	{
		case pProperties:
			err = AECreateList(NULL, 0L, true, data);
			ThrowIfOSErr(err);

			err = AEPutKeyPtr(data, pObjectType, typeType, &aType, sizeof(DescType));

			GetCleanedWindowName(window, windowTitle, 255);
			err = AEPutKeyPtr(data, pTitle,  typeChar, windowTitle, strlen(windowTitle));
			
			GetWindowUrlString(window, &urlString);
			if (urlString)
			{
				err = AEPutKeyPtr(data, AE_www_typeWindowURL,  typeChar, urlString, strlen(urlString));
				nsMemory::Free(urlString); urlString = NULL;
			}
			
			index = GetWindowIndex(GetThisWindowKind(), window);
			err = AEPutKeyPtr(data, pIndex, typeLongInteger, &index, sizeof(long));

			GetWindowGlobalBounds(window, &aRect);
			err = AEPutKeyPtr(data, pBounds, typeQDRectangle, &aRect, sizeof(Rect));
			aPoint.h = aRect.left;
			aPoint.v = aRect.top;
			err = AEPutKeyPtr(data, pLocation, typeQDPoint, &aPoint, sizeof(Point));
			
			aBoolean = WindowIsCloseable(window);
			err = AEPutKeyPtr(data, pHasCloseBox, typeBoolean, &aBoolean, sizeof(Boolean));

			aBoolean = WindowHasTitleBar(window);
			err = AEPutKeyPtr(data, pHasTitleBar, typeBoolean, &aBoolean, sizeof(Boolean));

			aBoolean = WindowIsModal(window);
			err = AEPutKeyPtr(data, pIsModal, typeBoolean, &aBoolean, sizeof(Boolean));

			aBoolean = WindowIsResizeable(window);
			err = AEPutKeyPtr(data, pIsResizable, typeBoolean, &aBoolean, sizeof(Boolean));

			aBoolean = WindowIsZoomable(window);
			err = AEPutKeyPtr(data, pIsZoomable, typeBoolean, &aBoolean, sizeof(Boolean));

			aBoolean = WindowIsZoomed(window);
			err = AEPutKeyPtr(data, pIsZoomed, typeBoolean, &aBoolean, sizeof(Boolean));
			
			aBoolean = !WindowIsModal(window);
			err = AEPutKeyPtr(data, pIsModeless, typeBoolean, &aBoolean, sizeof(Boolean));
			
			aBoolean = WindowIsModal(window) && WindowHasTitleBar(window);
			err = AEPutKeyPtr(data, pIsMovableModal, typeBoolean, &aBoolean, sizeof(Boolean));
			
			aBoolean = WindowIsFloating(window);
			err = AEPutKeyPtr(data, pIsFloating, typeBoolean, &aBoolean, sizeof(Boolean));
			
			aBoolean = IsWindowVisible(window);
			err = AEPutKeyPtr(data, pVisible, typeBoolean, &aBoolean, sizeof(Boolean));
			break;
							
		case pBestType:
		case pClass:
		case pDefaultType:
		case pObjectType:
			err = AECreateDesc(typeType, &aType, sizeof(DescType), data);
			break;
					
		case pBounds:
			GetWindowGlobalBounds(window, &aRect);
			err = AECreateDesc(typeQDRectangle, &aRect, sizeof(Rect), data);
			break;
			
		case pLocation:
			GetWindowGlobalBounds(window, &aRect);
			aPoint.h = aRect.left;
			aPoint.v = aRect.top;
			err = AECreateDesc(typeQDPoint, &aPoint, sizeof(Point), data);
			break;
			
		case pIsModeless:
			aBoolean = !WindowIsModal(window);
			err = AECreateDesc(typeBoolean, &aBoolean, sizeof(Boolean), data);
			break;
			
		case pIsMovableModal:
			aBoolean = WindowIsModal(window) && WindowHasTitleBar(window);
			err = AECreateDesc(typeBoolean, &aBoolean, sizeof(Boolean), data);
			break;
											
		case pHasCloseBox:
			aBoolean = WindowIsCloseable(window);
			err = AECreateDesc(typeBoolean, &aBoolean, sizeof(Boolean), data);
			break;
			
		case pHasTitleBar:
			aBoolean = WindowHasTitleBar(window);
			err = AECreateDesc(typeBoolean, &aBoolean, sizeof(Boolean), data);
			break;
			
		case pIndex:
			index = GetWindowIndex(GetThisWindowKind(), window);
			err = AECreateDesc(typeLongInteger, &index, sizeof(long), data);
			break;
							
		case pIsModal:
			aBoolean = WindowIsModal(window);
			err = AECreateDesc(typeBoolean, &aBoolean, sizeof(Boolean), data);
			break;
			
		case pIsResizable:
			aBoolean = WindowIsResizeable(window);
			err = AECreateDesc(typeBoolean, &aBoolean, sizeof(Boolean), data);
			break;
			
		case pIsZoomable:
			aBoolean = WindowIsZoomable(window);
			err = AECreateDesc(typeBoolean, &aBoolean, sizeof(Boolean), data);
			break;
			
		case pIsZoomed:
			aBoolean = WindowIsZoomed(window);
			err = AECreateDesc(typeBoolean, &aBoolean, sizeof(Boolean), data);
			break;
			
		case pVisible:
			aBoolean = IsWindowVisible(window);
			err = AECreateDesc(typeBoolean, &aBoolean, sizeof(Boolean), data);
			break;
		
		case pIsFloating:
			aBoolean = WindowIsFloating(window);
			err = AECreateDesc(typeBoolean, &aBoolean, sizeof(Boolean), data);
			break;
			
		case pName:		
		case pTitle:
			GetCleanedWindowName(window, windowTitle, 255);
			err = AECreateDesc(typeChar, windowTitle, strlen(windowTitle), data);
			break;
							
		case AE_www_typeWindowURL:
		   GetWindowUrlString(window, &urlString);
		   if (urlString)
		   {
			   err = AECreateDesc(typeChar, urlString, strlen(urlString), data);
			   nsMemory::Free(urlString); urlString = NULL;
			 }
		   break;
							
		default:
			Inherited::GetDataFromObject(token, desiredTypes, data);
			break;
	}
	
	ThrowIfOSErr(err);
}






void AEWindowClass::SetDataForObject(const AEDesc *token, AEDesc *data)
{
	OSErr		err;
	
	ConstAETokenDesc	tokenDesc(token);

	Boolean		usePropertyCode 	= tokenDesc.UsePropertyCode();
	WindowPtr	window 			= tokenDesc.GetWindowPtr();
		
	if (usePropertyCode == false)
	{
		ThrowIfOSErr(errAEWriteDenied);
	}
	else
	{
		DescType	propertyCode = tokenDesc.GetPropertyCode();
		
		

		if (data->descriptorType == typeAERecord)
		{		
			SetWindowProperties(window, data);
		}
		else	
		{
			StAEDesc		propertyRecord;
			
			err = AECreateList(nil, 0, true, &propertyRecord);
			ThrowIfOSErr(err);
			
			err = AEPutKeyDesc(&propertyRecord, propertyCode, data);
			ThrowIfOSErr(err);
		
			SetWindowProperties(window, &propertyRecord);
		}
	}
}

#pragma mark -






Boolean AEWindowClass::CanSetProperty(DescType propertyCode)
{
	Boolean	result = false;
		
	switch (propertyCode)
	{
		
						
		case pProperties:
			result = true;
			break;
			
		
		
		case pTitle:
		case pName:	

		case pBounds:
		case pLocation:
		case pIndex:
		case pVisible:
		case pIsZoomed:
			result = true;
			break;
		
		
		case AE_www_typeWindowURL:
			result = false;
			break;

		

		default:
		case pBestType:
		case pClass:
		case pDefaultType:
		case pObjectType:
				
		case pIsModeless:
		case pIsMovableModal:
		case pIsSuspended:
		case pIsPalette:
		case pIsDialog:

		case pHasCloseBox:
		case pHasTitleBar:

		case pIsFloating:
		case pIsModal:
		case pIsResizable:
		case pIsZoomable:
			result = Inherited::CanSetProperty(propertyCode);
			break;
	}

	return result;
}







Boolean AEWindowClass::CanGetProperty(DescType propertyCode)
{
	Boolean	result = false;
	
	switch (propertyCode)
	{
		
		
		case pBestType:
		case pClass:
		case pDefaultType:
		case pObjectType:
		
		case pContents:
		
		case pProperties:
		case keyAEProperties:
		
		case pTitle:
		case pName:	
		
		case AE_www_typeWindowURL:
		
		case pIsModeless:
		case pIsMovableModal:
		case pIsSuspended:
		case pIsPalette:
		case pIsDialog:
		
		case pBounds:
		case pLocation:

		case pHasCloseBox:
		case pHasTitleBar:
		case pIndex:

		case pIsFloating:
		case pIsModal:
		case pIsResizable:
		case pIsZoomable:

		case pIsZoomed:
		case pVisible:
			result = true;
			break;
			
		
		default:
			result = Inherited::CanGetProperty(propertyCode);
			break;
	}

	return result;
}

#pragma mark -






void AEWindowClass::SetWindowProperties(WindowPtr window, const AEDesc *propertyRecord)
{
	OSErr 				err 			= noErr;
	OSErr 				ignoreError 	= noErr;
	long 					numProperties 	= 0L;
	StAEDesc 				data;
	
	if (propertyRecord == nil || propertyRecord->descriptorType != typeAERecord)
	{
		ThrowIfOSErr(errAEWrongDataType);
	}
				
	err = AECountItems(propertyRecord, &numProperties);
	ThrowIfOSErr(err);
	if (numProperties <= 0)
		ThrowIfOSErr(paramErr);
		
	StPortSetter	portSetter(window);

	
	
	ignoreError = AEGetKeyDesc(propertyRecord, pBounds, typeQDRectangle, &data);
	if (ignoreError != errAEDescNotFound)
	{
		Rect r;
		err = DescToRect(&data, &r);

		if (err == noErr)
		{		
 			MoveWindow(window, r.left, r.top, false);
			
			
 			
		}
					
		data.Clear();
	}

	
	
	ignoreError = AEGetKeyDesc(propertyRecord, pLocation, typeQDPoint, &data);
	if (ignoreError != errAEDescNotFound)
	{
		Point p;
		err = DescToPoint(&data, &p);

		if (err == noErr)
			MoveWindow(window, p.h, p.v, false);  

		data.Clear();
	}

	
	
	ignoreError = AEGetKeyDesc(propertyRecord, pIndex, typeLongInteger, &data);
	if (ignoreError != errAEDescNotFound)
	{
		long 			index;
		long 			numWindows;
		WindowPtr 	behindWindow;
		
		err = DescToLong(&data, &index);

		if (err == noErr)
		{	
			numWindows = CountWindows(GetThisWindowKind());
			if (index < 0)
				index = numWindows + index + 1;
			
			if (index == 1)
			{
				SelectWindow(window);
			}
			else
			{
				behindWindow = GetWindowByIndex(GetThisWindowKind(), index);
				if (behindWindow != NULL)
				{
					SendBehind(window, behindWindow);
				}
				else
				{
					err = errAEEventNotHandled;
				}
			}
		}					

		data.Clear();
	}

	
	
	ignoreError = AEGetKeyDesc(propertyRecord, pIsZoomed, typeBoolean, &data);
	if (ignoreError != errAEDescNotFound)
	{
		Boolean 	zoomIt;
		
		err = DescToBoolean(&data, &zoomIt);
		
		if (err == noErr)
		{
			if (zoomIt)
				ZoomWindow(window, inZoomOut, false);
			else
				ZoomWindow(window, inZoomIn, false);
		}					

		data.Clear();
	}

	
	
	ignoreError = AEGetKeyDesc(propertyRecord, pVisible, typeBoolean, &data);
	if (ignoreError != errAEDescNotFound)
	{
		Boolean showIt;
		err = DescToBoolean(&data, &showIt);
		
		if (err == noErr)
		{
			if (showIt)
				ShowWindow(window);
			else
				HideWindow(window);
		}					

		data.Clear();
	}
}

#pragma mark -







void AEWindowClass::MakeWindowObjectSpecifier(WindowPtr wind, AEDesc *outSpecifier)
{
	
	CoreTokenRecord	coreToken;
	StAEDesc			windowToken;
	
	coreToken.window = wind;
	
	OSErr	err = ::AECreateDesc(typeObjectSpecifier, &coreToken, sizeof(CoreTokenRecord), &windowToken);
	ThrowIfOSErr(err);
		
	CreateSelfSpecifier(&windowToken, outSpecifier);
}






void AEWindowClass::CreateSelfSpecifier(const AEDesc *token, AEDesc *outSpecifier)
{
	ConstAETokenDesc	tokenDesc(token);
	WindowPtr	window = tokenDesc.GetWindowPtr();
	long			position = GetWindowIndex(mWindowKind, window);
	AEDesc		selfDesc = { typeNull, nil };
	AEDesc		containerSpecifier = { typeNull, nil };
	DescType		desiredClass = mClass;
	DescType		keyForm = formAbsolutePosition;
	OSErr		err = noErr;
	
	GetContainerSpecifier(token, &containerSpecifier);
	
	err = ::AECreateDesc(typeLongInteger, &position, sizeof(long), &selfDesc);
	ThrowIfOSErr(err);
	
	err = ::CreateObjSpecifier(desiredClass, &containerSpecifier, keyForm, &selfDesc, true, outSpecifier);
	ThrowIfOSErr(err);
}

#pragma mark -





long AEWindowClass::CountWindows(TWindowKind windowKind)
{
	return CountWindowsOfKind(windowKind);
}






WindowPtr AEWindowClass::GetWindowByIndex(TWindowKind windowKind, long index)
{
	return GetIndexedWindowOfKind(windowKind, index);
}





WindowPtr AEWindowClass::GetWindowByTitle(TWindowKind windowKind, ConstStr63Param title)
{
	CStr255		windowName;
	CopyPascalToCString((const StringPtr)title, windowName, 255);
	return GetNamedOrFrontmostWindow(windowKind, windowName);
}





long AEWindowClass::GetWindowIndex(TWindowKind windowKind, WindowPtr window)
{
	return nsWindowUtils::GetWindowIndex(windowKind, window);
}






WindowPtr AEWindowClass::GetPreviousWindow(TWindowKind windowKind, WindowPtr wind)
{
	short	windowIndex = GetWindowIndex(windowKind, wind);
	return GetIndexedWindowOfKind(windowKind, windowIndex - 1);
}


