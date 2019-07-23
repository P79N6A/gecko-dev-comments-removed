






































#include "nsAEUtils.h"
#include "nsAETokens.h"
#include "nsAECoreClass.h"

#include "nsAEGenericClass.h"






AEGenericClass::AEGenericClass(DescType classType, DescType containerClass)
:	mClass(classType)
,	mContainerClass(containerClass)
,	mItemFromContainerAccessor(nil)
{

	
	mItemFromContainerAccessor = NewOSLAccessorUPP(AEGenericClass::ItemFromContainerAccessor);
	ThrowIfNil(mItemFromContainerAccessor);
	
	OSErr	err;
	err = AEInstallObjectAccessor(mClass,	 	containerClass, 
										mItemFromContainerAccessor, 
										(long)this, 
										false);

	
	
	
	
	err = AEInstallObjectAccessor(mClass, 		mClass, 
										mItemFromContainerAccessor, 
										(long)this, 
										false);
	ThrowIfOSErr(err);

}





AEGenericClass::~AEGenericClass()
{
	if (mItemFromContainerAccessor)
		DisposeOSLAccessorUPP(mItemFromContainerAccessor);
}

#pragma mark -







void AEGenericClass::DispatchEvent(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	OSErr		err = noErr;
	
	AEEventID		eventID;
	OSType		typeCode;
	Size			actualSize 	= 0L;
	
	
	err = AEGetAttributePtr(appleEvent, 	keyEventIDAttr, 
									typeType, 
									&typeCode, 
									(Ptr)&eventID, 
									sizeof(eventID), 
									&actualSize);
	ThrowIfOSErr(err);
	
	try
	{
		switch (eventID)
		{
			case kAEClone:
				HandleDuplicate(token, appleEvent, reply);
				break;
				
			case kAEClose:
				HandleClose(token, appleEvent, reply);
				break;
				
			case kAECountElements:
				HandleCount(token, appleEvent, reply);
				break;
				
			case kAECreateElement:
				HandleMake(token, appleEvent, reply);
				break;
				
			case kAEDelete:
				HandleDelete(token, appleEvent, reply);
				break;
				
			case kAEDoObjectsExist:
				HandleExists(token, appleEvent, reply);
				break;
				
			case kAEGetData:
				HandleGetData(token, appleEvent, reply);
				break;
				
			case kAEGetDataSize:
				HandleDataSize(token, appleEvent, reply);
				break;
				
			case kAEMove:
				HandleMove(token, appleEvent, reply);
				break;
				
			case kAEOpen:		
				HandleOpen(token, appleEvent, reply);
				break;
				
			case kAEPrint:
				HandlePrint(token, appleEvent, reply);
				break;
			
			case kAEOpenApplication:
				HandleRun(token, appleEvent, reply);
				break;
			
			case kAEReopenApplication:
			  HandleReOpen(token, appleEvent, reply);
			  break; 
							
			case kAEQuitApplication:
				HandleQuit(token, appleEvent, reply);
				break;
				
			case kAESave:
				HandleSave(token, appleEvent, reply);
				break;
				
			case kAESetData:
				HandleSetData(token, appleEvent, reply);
				break;

			
			case kAEExtract:
				HandleExtract(token, appleEvent, reply);
				break;
				
			case kAESendMessage:
				HandleSendMessage(token, appleEvent, reply);
				break;
				
			default:
				err = errAEEventNotHandled;
				break;
		}
	}
	catch (OSErr catchErr)
	{
		PutReplyErrorNumber(reply, catchErr);
		throw;
	}
	catch ( ... )
	{
		PutReplyErrorNumber(reply, paramErr);
		throw;
	}
}






void	AEGenericClass::GetProperty(				DescType			desiredClass,
										const AEDesc*		containerToken,
										DescType			containerClass,
										DescType			keyForm,
										const AEDesc*		keyData,
										AEDesc*			resultToken)
{
	
	
	GetPropertyFromListOrObject(desiredClass, containerToken, containerClass, keyForm, keyData, resultToken);
}

#pragma mark -






pascal OSErr AEGenericClass::ItemFromContainerAccessor(	DescType			desiredClass,		
												const AEDesc*		containerToken,	
												DescType			containerClass,  	 
												DescType			keyForm,
												const AEDesc*		keyData,
												AEDesc*			resultToken,		
												long 				refCon)
{
	AEGenericClass*	itemClass = reinterpret_cast<AEGenericClass *>(refCon);
	if (!itemClass) return paramErr;
	
	OSErr		err = noErr;
	
	try
	{
		itemClass->GetItemFromContainer(desiredClass, containerToken, containerClass, keyForm, keyData, resultToken);
	}
	catch(OSErr catchErr)
	{
		err = catchErr;
	}
	catch(...)
	{
		err = paramErr;
	}
	
	return err;
}



#pragma mark -





void AEGenericClass::HandleClose(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowIfOSErr(errAEEventNotHandled);
}





void AEGenericClass::HandleCount(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ConstAETokenDesc	tokenDesc(token);
	long 			numberOfObjects = 0;
	DescType		objectClass;
	OSErr		err = noErr;	

	if (!reply->dataHandle)
		return;
	
	
	err = GetObjectClassFromAppleEvent(appleEvent, &objectClass);
	ThrowIfOSErr(err);
	
	err = CheckForUnusedParameters(appleEvent);
	ThrowIfOSErr(err);

	if (AEListUtils::TokenContainsTokenList(token))
	{
		err = AECountItems(token, &numberOfObjects);
		ThrowIfOSErr(err);
		
	}
	else
	{
		CountObjects(objectClass, tokenDesc.GetDispatchClass(), token, &numberOfObjects);
	}

	err = AEPutParamPtr(reply, keyAEResult, 
								 typeLongInteger, 
								 (Ptr)&numberOfObjects, 
								 sizeof(long));
	ThrowIfOSErr(err);
}






void AEGenericClass::HandleGetData(AEDesc *tokenOrTokenList, const AppleEvent *appleEvent, AppleEvent *reply)
{
	OSErr 		err 			= noErr;	
	StAEDesc		data;
	StAEDesc		desiredTypes;
	
	(void)AEGetParamDesc(appleEvent, keyAERequestedType, typeAEList, &desiredTypes);
	
	GetDataFromListOrObject(tokenOrTokenList, &desiredTypes, &data);

	if (reply->descriptorType != typeNull)
	{
		err = AEPutKeyDesc(reply, keyDirectObject, &data);
		ThrowIfOSErr(err);
	}
}






void AEGenericClass::HandleSetData(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	SetDataForListOrObject(token, appleEvent, reply);
}





void AEGenericClass::HandleDataSize(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowIfOSErr(errAEEventNotHandled);
}








void AEGenericClass::HandleDelete(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowIfOSErr(errAEEventNotHandled);
}





void AEGenericClass::HandleDuplicate(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowIfOSErr(errAEEventNotHandled);
}










void AEGenericClass::HandleExists(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	OSErr	err;
	Boolean	foundIt	= true;

	err = AEPutParamPtr(reply, 
					 keyAEResult, 
					 typeBoolean, 
					 (Ptr)&foundIt, 
					 sizeof(Boolean));
		
	ThrowIfOSErr(err);
}






void AEGenericClass::HandleMake(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	DescType		insertionPos	= typeNull;
	OSErr		err 			= noErr;

	StAEDesc		insertionLoc;
	StAEDesc		objectSpec;

	
	
	
	
	
	
	err = ::AEGetParamDesc(appleEvent, 				
								  keyAEInsertHere, 
								  typeAERecord, 
								  &insertionLoc);
	if (err == errAEDescNotFound)
	{
		insertionPos	= kAEBeginning;
		err			= noErr;
	}
	else if (err == noErr)
	{
		
		
		OSType		typeCode;
		Size			actualSize;
		err = ::AEGetKeyPtr(&insertionLoc, 
								  	keyAEPosition, 			
									typeEnumeration, 
									&typeCode, 
									(Ptr)&insertionPos,
						   			sizeof(insertionPos), 
						   			&actualSize);

		
		
		
				 
		err = ::AEGetKeyDesc(&insertionLoc, 
									keyAEObject, 
									typeWildCard, 
									&objectSpec);
	}

	
	
	
	
	if (objectSpec.descriptorType == typeNull) 
	{
		::AEDisposeDesc(token);			
	}
	else
	{
		err = ::AEResolve(&objectSpec, 
							 kAEIDoMinimum, 
							 token);			
		ThrowIfOSErr(err);
	}
		
	
	
	
	
	StAEDesc		withData;
	const AEDesc*	withDataPtr = nil;
	
	err = ::AEGetParamDesc(appleEvent, 
								  keyAEData,
								  typeWildCard,
								  &withData);
	if (err == errAEDescNotFound)
		err = noErr;
	else
	{
		ThrowIfOSErr(err);
		withDataPtr = &withData;
	}

	
	StAEDesc		withProperties;
	const AEDesc*	withPropertiesPtr = nil;
	err = AEGetParamDesc(appleEvent, 
								  keyAEPropData, 
								  typeWildCard, 
								  &withProperties);
								  
	if (err == errAEDescNotFound)
		err = noErr;
	else
	{
		ThrowIfOSErr(err);
		withPropertiesPtr = &withProperties;
	}
	
	
	MakeNewObject(insertionPos, token, withDataPtr, withPropertiesPtr, reply);
}






void AEGenericClass::HandleMove(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowIfOSErr(errAEEventNotHandled);
}






void AEGenericClass::HandleOpen(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowIfOSErr(errAEEventNotHandled);
}






void AEGenericClass::HandleReOpen(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowIfOSErr(errAEEventNotHandled);
}






void AEGenericClass::HandleRun(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowIfOSErr(errAEEventNotHandled);
}






void AEGenericClass::HandlePrint(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowIfOSErr(errAEEventNotHandled);
}






void AEGenericClass::HandleQuit(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowIfOSErr(errAEEventNotHandled);
}






void AEGenericClass::HandleSave(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowOSErr(errAEEventNotHandled);
}







void AEGenericClass::HandleExtract(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowOSErr(errAEEventNotHandled);
}







void AEGenericClass::HandleSendMessage(AEDesc *token, const AppleEvent *appleEvent, AppleEvent *reply)
{
	ThrowOSErr(errAEEventNotHandled);
}


#pragma mark -





void AEGenericClass::CompareObjects(				DescType			comparisonOperator,
											const AEDesc *		object,
											const AEDesc *		descriptorOrObject,
											Boolean *			result)
{
	ThrowOSErr(errAEEventNotHandled);
}





void AEGenericClass::CountObjects(					DescType 		 	desiredType,
											DescType 		 	containerClass,
											const AEDesc *		container,
							   				long *			result)
{
	ThrowOSErr(errAEEventNotHandled);
}

#pragma mark -







void AEGenericClass::GetDataFromListOrObject(const AEDesc *tokenOrTokenList, AEDesc *desiredTypes, AEDesc *data)
{
	if (AEListUtils::TokenContainsTokenList(tokenOrTokenList) == false)
	{
		GetDataFromObject(tokenOrTokenList, desiredTypes, data);
	}
	else
	{
		ThrowIfOSErr(AECreateList(nil, 0, false, data));
		GetDataFromList(tokenOrTokenList, desiredTypes, data);
	}
}







void AEGenericClass::SetDataForListOrObject(const AEDesc *tokenOrTokenList, const AppleEvent *appleEvent, AppleEvent *reply)
{
	StAEDesc			data;
	
	switch (tokenOrTokenList->descriptorType)
	{
		case typeAEList:
			{
				AECoreClass::GetAECoreHandler()->GetEventKeyDataParameter(appleEvent, typeWildCard, &data);
				SetDataForList(tokenOrTokenList, &data);
			}
			break;
				
		case cProperty:
			{
				ConstAETokenDesc	tokenDesc(tokenOrTokenList);
				DescType		propertyCode = tokenDesc.GetPropertyCode();
				
				
				if (CanSetProperty(propertyCode))
				{
					AECoreClass::GetAECoreHandler()->GetEventKeyDataParameter(appleEvent, GetKeyEventDataAs(propertyCode), &data);
					SetDataForObject(tokenOrTokenList, &data);
				}
				else
				{
					ThrowIfOSErr(errAENotModifiable);
				}
			}
			break;
			
		default:
			ThrowIfOSErr(errAENotModifiable);
	}
}






void AEGenericClass::GetDataFromList(const AEDesc *srcList, AEDesc *desiredTypes, AEDesc *dstList)
{
	OSErr		err;
	long			itemNum;
	long			numItems;
	DescType		keyword;
	StAEDesc		srcItem;
	StAEDesc		dstItem;
		
	err = AECountItems((AEDescList*)srcList, &numItems);
	ThrowIfOSErr(err);
		
	for (itemNum = 1; itemNum <= numItems; itemNum++)
	{
		err = AEGetNthDesc(srcList, itemNum, typeWildCard, &keyword, &srcItem);
		ThrowIfOSErr(err);
		
		if (AEListUtils::TokenContainsTokenList(&srcItem) == false)
		{
			GetDataFromObject(&srcItem, desiredTypes, &dstItem);  
		}
		else
		{
			ThrowIfOSErr(AECreateList(nil, 0, false, &dstItem));
			GetDataFromList(&srcItem, desiredTypes, &dstItem);
		}
		err = AEPutDesc(dstList, itemNum, &dstItem);
		ThrowIfOSErr(err);
	}
}







void AEGenericClass::GetDataFromObject(const AEDesc *token, AEDesc *desiredTypes, AEDesc *data)
{
	ConstAETokenDesc		tokenDesc(token);
	DescType 			propertyCode 		= tokenDesc.GetPropertyCode();
	OSErr			err				= noErr;

	switch (propertyCode)
	{
		case pContents:
		case typeNull:
			
			CreateSpecifier(token, data);
			break;
			
		default:
			err = errAECantSupplyType;
			break;
	}
	
	ThrowIfOSErr(err);
}









void AEGenericClass::SetDataForList(const AEDesc *token, AEDesc *data)
{
	OSErr 			err;
		
	if (AEListUtils::TokenContainsTokenList(token) == false)
	{
		SetDataForObject(token, data);
	}
	else
	{
		long			numItems;
		long			itemNum;
		err = AECountItems((AEDescList*)token, &numItems);
		ThrowIfOSErr(err);
		
		for (itemNum = 1; itemNum <= numItems; itemNum++)
		{
			StAEDesc	  	tempToken;
			AEKeyword	keyword;
			
		 	err = AEGetNthDesc((AEDescList*)token, itemNum, typeWildCard, &keyword, &tempToken);
			ThrowIfOSErr(err);
			
			if (AEListUtils::TokenContainsTokenList(&tempToken) == false)
			{
				SetDataForObject(&tempToken, data);  		
			}
			else
			{
				SetDataForList(&tempToken, data); 	
			}
		}
	}
}






DescType AEGenericClass::GetKeyEventDataAs(DescType propertyCode)
{
	return typeWildCard;
}

#pragma mark -





Boolean AEGenericClass::CanGetProperty(DescType propertyCode)
{
	Boolean	canGet = false;
	
	switch (propertyCode)
	{
		case pContents:
			canGet = true;
			break;
	}
	return canGet;
}





Boolean AEGenericClass::CanSetProperty(DescType propertyCode)
{
	return false;
}


#pragma mark -






void AEGenericClass::CreateSpecifier(const AEDesc *token, AEDesc *outSpecifier)
{
	CreateSelfSpecifier(token, outSpecifier);
}





void AEGenericClass::GetContainerSpecifier(const AEDesc *token, AEDesc *outContainerSpecifier)
{
	outContainerSpecifier->descriptorType = typeNull;
	outContainerSpecifier->dataHandle = nil;
	
	AEDispatchHandler*	handler = AECoreClass::GetDispatchHandlerForClass(mContainerClass);
	if (handler)
	{
		handler->CreateSelfSpecifier(token, outContainerSpecifier);
	}
}

#pragma mark -





void AEGenericClass::GetPropertyFromListOrObject(		DescType			desiredClass,
												const AEDesc*		containerToken,
												DescType			containerClass,
												DescType			keyForm,
												const AEDesc*		keyData,
												AEDesc*			resultToken)
{
	if (AEListUtils::TokenContainsTokenList((AEDescList*)containerToken) == false)
	{
		GetPropertyFromObject(desiredClass, containerToken, containerClass, keyForm, keyData, resultToken);
	}
	else
	{
		OSErr	err = AECreateList(nil, 0, false, resultToken);
		ThrowIfOSErr(err);
		
		GetPropertyFromList(desiredClass, containerToken, containerClass, keyForm, keyData, resultToken);
	}
}






void AEGenericClass::GetPropertyFromList(				DescType			desiredClass,
												const AEDesc*		containerToken,
												DescType			containerClass,
												DescType			keyForm,
												const AEDesc*		keyData,
												AEDesc*			resultToken)
{
	OSErr		err		= noErr;
	long			itemNum;
	long			numItems;
	DescType		keyword;
	
	err = AECountItems((AEDescList*)containerToken, &numItems);
	ThrowIfOSErr(err);
		
	for (itemNum = 1; itemNum <= numItems; itemNum++)
	{
		StAEDesc		srcItem;
		StAEDesc		dstItem;
		
		err = AEGetNthDesc(containerToken, itemNum, typeWildCard, &keyword, &srcItem);
		ThrowIfOSErr(err);
		
		if (AEListUtils::TokenContainsTokenList(&srcItem) == false)
		{
			GetPropertyFromObject(desiredClass, &srcItem, containerClass, keyForm, keyData, &dstItem);
		}
		else
		{
			err = AECreateList(nil, 0, false, &dstItem);
			ThrowIfOSErr(err);
			
			GetPropertyFromList(desiredClass, &srcItem, containerClass, keyForm, keyData, &dstItem);
		}

		err = AEPutDesc(resultToken, itemNum, &dstItem);
		ThrowIfOSErr(err);
	}
	
}






void AEGenericClass::GetPropertyFromObject(			DescType			desiredType,
						  					const AEDesc*		containerToken,
								  			DescType			containerClass,
								  			DescType			keyForm,
								 			const AEDesc*		keyData,
								  			AEDesc*			resultToken)
{
	OSErr			err;	
	DescType			requestedProperty;

	err = AEDuplicateDesc(containerToken, resultToken);
	ThrowIfOSErr(err);
		
	requestedProperty = **(DescType**)(keyData->dataHandle);
	
	if (requestedProperty == kAEAll || requestedProperty == keyAEProperties)
		requestedProperty = pProperties;

	if (CanGetProperty(requestedProperty) || CanSetProperty(requestedProperty))
	{
		AETokenDesc		resultTokenDesc(resultToken);
		resultToken->descriptorType = desiredType;
		resultTokenDesc.SetPropertyCode(requestedProperty);
	}
	else
	{
		ThrowIfOSErr(errAEEventNotHandled);
	}
}


#pragma mark -






void AEGenericClass::MakeNewObject(				const DescType		insertionPosition,
											const AEDesc*		token,
											const AEDesc*		ptrToWithData, 
											const AEDesc*		ptrToWithProperties,
											AppleEvent*		reply)
{
	ThrowOSErr(errAEEventNotHandled);
}

