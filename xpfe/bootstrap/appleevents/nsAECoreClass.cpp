





































#include "nsAEClassTypes.h"
#include "nsAEUtils.h"
#include "nsAETokens.h"
#include "nsAECompare.h"

#include "nsAEApplicationClass.h"
#include "nsAEDocumentClass.h"
#include "nsAEWindowClass.h"







#include "nsAECoreClass.h"

AECoreClass*		AECoreClass::sAECoreHandler = nil;






AECoreClass::AECoreClass(Boolean suspendEvents)
:	mSuspendEventHandlerUPP(nil)
,	mStandardSuiteHandlerUPP(nil)
,	mRequiredSuiteHandlerUPP(nil)
,	mCreateElementHandlerUPP(nil)
,	mMozillaSuiteHandlerUPP(nil)
,	mGetURLSuiteHandlerUPP(nil)
,	mSpyGlassSuiteHandlerUPP(nil)
,	mPropertyFromListAccessor(nil)
,	mAnythingFromAppAccessor(nil)
,	mCountItemsCallback(nil)
,	mCompareItemsCallback(nil)
{
	mSuspendedEvent.descriptorType = typeNull;
	mSuspendedEvent.dataHandle = nil;

	mReplyToSuspendedEvent.descriptorType = typeNull;
	mReplyToSuspendedEvent.dataHandle = nil;
	
	OSErr	err = ::AEObjectInit();
	ThrowIfOSErr(err);
	
	mSuspendEventHandlerUPP = NewAEEventHandlerUPP(AECoreClass::SuspendEventHandler);
	ThrowIfNil(mSuspendEventHandlerUPP);

	mRequiredSuiteHandlerUPP = NewAEEventHandlerUPP(AECoreClass::RequiredSuiteHandler);
	ThrowIfNil(mRequiredSuiteHandlerUPP);

	mStandardSuiteHandlerUPP = NewAEEventHandlerUPP(AECoreClass::CoreSuiteHandler);
	ThrowIfNil(mStandardSuiteHandlerUPP);

	mMozillaSuiteHandlerUPP = NewAEEventHandlerUPP(AECoreClass::MozillaSuiteHandler);
	ThrowIfNil(mMozillaSuiteHandlerUPP);

	mGetURLSuiteHandlerUPP = NewAEEventHandlerUPP(AECoreClass::GetURLSuiteHandler);
	ThrowIfNil(mGetURLSuiteHandlerUPP);

	mSpyGlassSuiteHandlerUPP = NewAEEventHandlerUPP(AECoreClass::SpyglassSuiteHandler);
	ThrowIfNil(mSpyGlassSuiteHandlerUPP);

	InstallSuiteHandlers(suspendEvents);

	mCreateElementHandlerUPP = NewAEEventHandlerUPP(AECoreClass::CreateElementHandler);
	ThrowIfNil(mCreateElementHandlerUPP);
	
	
	
	
	

	err = ::AEInstallEventHandler(kAECoreSuite, 	kAECreateElement, 
										mCreateElementHandlerUPP, 
										(long)this, 
										false);
	ThrowIfOSErr(err);

	




	
	mAnythingFromAppAccessor = NewOSLAccessorUPP(AECoreClass::AnythingFromAppAccessor);
	ThrowIfNil(mAnythingFromAppAccessor);
	
	
	err = ::AEInstallObjectAccessor(typeWildCard, typeWildCard, mAnythingFromAppAccessor, (long)this, false);
	ThrowIfOSErr(err);
	
	
	mPropertyFromListAccessor = NewOSLAccessorUPP(AECoreClass::PropertyTokenFromAnything);
	ThrowIfNil(mPropertyFromListAccessor);
	
	
	err = ::AEInstallObjectAccessor(cProperty, typeWildCard, mPropertyFromListAccessor, (long)this, false);
	ThrowIfOSErr(err);

	
	mCountItemsCallback = NewOSLCountUPP(AECoreClass::CountObjectsCallback);
	ThrowIfNil(mCountItemsCallback);

	mCompareItemsCallback = NewOSLCompareUPP(AECoreClass::CompareObjectsCallback);
	ThrowIfNil(mCompareItemsCallback);
	
	err = ::AESetObjectCallbacks(
						mCompareItemsCallback,
						mCountItemsCallback,
						(OSLDisposeTokenUPP)	nil, 
						(OSLGetMarkTokenUPP)	nil, 
						(OSLMarkUPP)			nil, 
						(OSLAdjustMarksUPP)	nil, 
						(OSLGetErrDescUPP)  	nil);
	ThrowIfOSErr(err);

	
	
	AEApplicationClass*		appDispatcher = new AEApplicationClass;
	RegisterClassHandler(cApplication, appDispatcher);
	RegisterClassHandler(typeNull, appDispatcher, true);

	AEDocumentClass*		docDispatcher = new AEDocumentClass;
	RegisterClassHandler(cDocument, docDispatcher);

	AEWindowClass*		windowDispatcher = new AEWindowClass(cWindow, kAnyWindowKind);
	RegisterClassHandler(cWindow, windowDispatcher);











}







AECoreClass::~AECoreClass()
{
	if (mSuspendEventHandlerUPP)
		DisposeAEEventHandlerUPP(mSuspendEventHandlerUPP);

	if (mStandardSuiteHandlerUPP)
		DisposeAEEventHandlerUPP(mStandardSuiteHandlerUPP);
		
	if (mRequiredSuiteHandlerUPP)
		DisposeAEEventHandlerUPP(mRequiredSuiteHandlerUPP);
			
	if (mMozillaSuiteHandlerUPP)
		DisposeAEEventHandlerUPP(mMozillaSuiteHandlerUPP);

	if (mGetURLSuiteHandlerUPP)
		DisposeAEEventHandlerUPP(mGetURLSuiteHandlerUPP);

	if (mSpyGlassSuiteHandlerUPP)
		DisposeAEEventHandlerUPP(mSpyGlassSuiteHandlerUPP);	
	
	if (mCreateElementHandlerUPP)
		DisposeAEEventHandlerUPP(mCreateElementHandlerUPP);
		
	if (mPropertyFromListAccessor)
		DisposeOSLAccessorUPP(mPropertyFromListAccessor);

	if (mAnythingFromAppAccessor)
		DisposeOSLAccessorUPP(mAnythingFromAppAccessor);

	if (mCountItemsCallback)
		DisposeOSLCountUPP(mCountItemsCallback);

	if (mCompareItemsCallback)
		DisposeOSLCompareUPP(mCompareItemsCallback);
}

#pragma mark -






void AECoreClass::HandleCoreSuiteEvent(const AppleEvent *appleEvent, AppleEvent *reply)
{
	StAEDesc		directParameter;
	StAEDesc		token;
	OSErr		err = noErr;
	
	
	err = ::AEGetKeyDesc(appleEvent, keyDirectObject, typeWildCard, &directParameter);
	ThrowIfOSErr(err);

	DescType	dispatchClass  	= typeNull;
	
	
	
	if (directParameter.descriptorType == typeNull) 
	{
		token = directParameter;			
	}
	else
	{
		
		
		
		
		 
		err = ::AEResolve(&directParameter, kAEIDoMinimum, &token);
	}

	if (err == errAENoSuchObject || err == errAEIllegalIndex)
	{
		
		
		
		
		
		
		AEEventID		eventID;
		OSType		typeCode;
		Size			actualSize = 0;		
		OSErr		eventError = ::AEGetAttributePtr(appleEvent, 
										  		 keyEventIDAttr, 
										  		 typeType, 
										  		 &typeCode, 
										  		 (Ptr)&eventID, 	
										  		 sizeof(eventID), 
										  		 &actualSize);
										  
		
		
		
		if (eventError == noErr && eventID == kAEDoObjectsExist)
		{
			Boolean	foundIt = false;
			(void)AEPutParamPtr(reply, keyAEResult, typeBoolean, (Ptr)&foundIt, sizeof(Boolean));
			
			
			
			
			ThrowNoErr();
		}
		
		ThrowIfOSErr(err);
		return;
	}

	ThrowIfOSErr(err);
	
	
	

	
	
	
	
	
	
	
	
	
	
	if (AEListUtils::TokenContainsTokenList(&token))
	{
		SInt32	numItems;
		
		err = AECountItems(&token, &numItems);
		
		if (numItems == 0)	
		{
			dispatchClass = typeNull;
		}
		else
		{
			StAEDesc	tempToken;
			err = AEListUtils::GetFirstNonListToken((AEDesc *)&token, &tempToken);
			if (err == noErr && tempToken.descriptorType != typeNull)
			{
				ConstAETokenDesc	tokenDesc(&tempToken);
				dispatchClass = tokenDesc.GetDispatchClass();
			}
			else
			{
				dispatchClass = typeNull;
				err = noErr;
			}
		}
	}
	else if (token.descriptorType == typeNull)	           
	{
		dispatchClass = typeNull;
	}
	else
	{
		ConstAETokenDesc	tokenDesc(&token);
		dispatchClass = tokenDesc.GetDispatchClass();
	}
	
	
	if (dispatchClass == cFile)
	{
		AEEventID eventID	= 0;
		OSType  typeCode 	= 0;
		Size   actualSize	= 0;
		
		AEGetAttributePtr(appleEvent, keyEventIDAttr,
                      typeType,
                      &typeCode,
                      (Ptr)&eventID, 
                      sizeof(eventID),
                      &actualSize);
	}


	AEDispatchHandler*	handler = GetDispatchHandler(dispatchClass);
	if (!handler)
		ThrowIfOSErr(errAEEventNotHandled);
		
	handler->DispatchEvent(&token, appleEvent, reply);
}






void AECoreClass::HandleRequiredSuiteEvent(const AppleEvent *appleEvent, AppleEvent *reply)
{
	StAEDesc	token;
	
	
	(void)::AEGetParamDesc(appleEvent, keyDirectObject, typeAEList, &token);
	
	AEDispatchHandler*	handler = GetDispatchHandler(cApplication);
	if (!handler)
		ThrowIfOSErr(errAEEventNotHandled);
		
	handler->DispatchEvent(&token, appleEvent, reply);
}






void AECoreClass::HandleCreateElementEvent(const AppleEvent *appleEvent, AppleEvent *reply)
{
	StAEDesc		token;
	OSErr		err = noErr;
	
 	
 	
	err = ::AEGetParamDesc(appleEvent, keyAEObjectClass, typeType, &token);
	ThrowIfOSErr(err);

	DescType	dispatchClass = **(DescType **)(token.dataHandle);

	AEDispatchHandler*	handler = GetDispatchHandler(dispatchClass);
	if (!handler)
		ThrowIfOSErr(errAEEventNotHandled);
		
	handler->DispatchEvent(&token, appleEvent, reply);
}






void AECoreClass::HandleEventSuspend(const AppleEvent *appleEvent, AppleEvent *reply)
{
	mSuspendedEvent = *appleEvent;
	mReplyToSuspendedEvent = *reply;
	OSErr	err = ::AESuspendTheCurrentEvent(appleEvent);
	ThrowIfOSErr(err);
}






void AECoreClass::PropertyTokenFromList(			DescType			desiredClass,
			 								const AEDesc*		containerToken,
			 								DescType			containerClass,
			 								DescType			keyForm,
		    									const AEDesc*		keyData,
			 								AEDesc*			resultToken)
{
	DescType		handlerClass;
	ConstAETokenDesc	containerDesc(containerToken);
	
	switch (containerClass)
	{
		case cProperty:
			
			
			handlerClass = containerDesc.GetDispatchClass();
			break;

		default:
			handlerClass = containerClass;
			break;
	
	}
	AEDispatchHandler*	handler = GetDispatchHandler(handlerClass);
	if (!handler)
		ThrowIfOSErr(errAEEventNotHandled);
		
	handler->GetProperty(		desiredClass,
							containerToken,
							containerClass,
							keyForm,
							keyData,
							resultToken);
}






void AECoreClass::GetAnythingFromApp(				DescType			desiredClass,
			 								const AEDesc*		containerToken,
			 								DescType			containerClass,
			 								DescType			keyForm,
		    									const AEDesc*		keyData,
			 								AEDesc*			resultToken)
{
	AEDispatchHandler*	handler = GetDispatchHandler(desiredClass);
	if (!handler)
		ThrowIfOSErr(errAEEventNotHandled);
		
	handler->GetItemFromContainer(		desiredClass,
									containerToken,
									containerClass,
									keyForm,
									keyData,
									resultToken);
}


#pragma mark -










pascal OSErr AECoreClass::SuspendEventHandler(const AppleEvent *appleEvent, AppleEvent *reply, SInt32 refCon)
{
	AECoreClass*	coreClass = reinterpret_cast<AECoreClass *>(refCon);
	OSErr		err = noErr;
	
	if (coreClass == nil)
		return errAECorruptData;
	
	try
	{
		coreClass->HandleEventSuspend(appleEvent, reply);
	}
	catch (OSErr catchErr)
	{
		err = catchErr;
	}
	catch ( ... )
	{
		err = paramErr;
	}
	
	return err;
}












pascal OSErr AECoreClass::RequiredSuiteHandler(const AppleEvent *appleEvent, AppleEvent *reply, SInt32 refCon)
{
	AECoreClass*	coreClass = reinterpret_cast<AECoreClass *>(refCon);
	OSErr		err = noErr;
	
	if (coreClass == nil)
		return errAECorruptData;
	
	try
	{
		coreClass->HandleRequiredSuiteEvent(appleEvent, reply);
	}
	catch (OSErr catchErr)
	{
		err = catchErr;
	}
	catch ( ... )
	{
		err = paramErr;
	}
	
	return err;
}












pascal OSErr AECoreClass::CoreSuiteHandler(const AppleEvent *appleEvent, AppleEvent *reply, SInt32 refCon)
{
	AECoreClass*	coreClass = reinterpret_cast<AECoreClass *>(refCon);
	OSErr		err = noErr;
	
	if (coreClass == nil)
		return errAECorruptData;
	
	try
	{
		coreClass->HandleCoreSuiteEvent(appleEvent, reply);
	}
	catch (OSErr catchErr)
	{
		err = catchErr;
	}
	catch ( ... )
	{
		err = paramErr;
	}
	
	return err;
}






pascal OSErr AECoreClass::CreateElementHandler(const AppleEvent *appleEvent, AppleEvent *reply, SInt32 refCon)
{
	AECoreClass*	coreClass = reinterpret_cast<AECoreClass *>(refCon);
	OSErr		err = noErr;
	
	if (coreClass == nil)
		return errAECorruptData;
	
	try
	{
		coreClass->HandleCreateElementEvent(appleEvent, reply);
	}
	catch (OSErr catchErr)
	{
		err = catchErr;
	}
	catch ( ... )
	{
		err = paramErr;
	}
	
	return err;
}

#pragma mark -





pascal OSErr AECoreClass::MozillaSuiteHandler(const AppleEvent *appleEvent, AppleEvent *reply, SInt32 refCon)
{
	AECoreClass*	coreClass = reinterpret_cast<AECoreClass *>(refCon);
	OSErr		err = noErr;
	
	if (coreClass == nil)
		return errAECorruptData;
	
	try
	{
		coreClass->HandleMozillaSuiteEvent(appleEvent, reply);
	}
	catch (OSErr catchErr)
	{
		err = catchErr;
	}
	catch ( ... )
	{
		err = paramErr;
	}
	
	return err;
}







pascal OSErr AECoreClass::GetURLSuiteHandler(const AppleEvent *appleEvent, AppleEvent *reply, SInt32 refCon)
{
	AECoreClass*	coreClass = reinterpret_cast<AECoreClass *>(refCon);
	OSErr		err = noErr;
	
	if (coreClass == nil)
		return errAECorruptData;
	
	try
	{
		coreClass->HandleGetURLSuiteEvent(appleEvent, reply);
	}
	catch (OSErr catchErr)
	{
		err = catchErr;
	}
	catch ( ... )
	{
		err = paramErr;
	}
	
	return err;
}







pascal OSErr AECoreClass::SpyglassSuiteHandler(const AppleEvent *appleEvent, AppleEvent *reply, SInt32 refCon)
{
	AECoreClass*	coreClass = reinterpret_cast<AECoreClass *>(refCon);
	OSErr		err = noErr;
	
	if (coreClass == nil)
		return errAECorruptData;
	
	try
	{
		coreClass->HandleSpyglassSuiteEvent(appleEvent, reply);
	}
	catch (OSErr catchErr)
	{
		err = catchErr;
	}
	catch ( ... )
	{
		err = paramErr;
	}
	
	return err;
}


#pragma mark -






void AECoreClass::HandleMozillaSuiteEvent(const AppleEvent *appleEvent, AppleEvent *reply)
{
	mMozillaSuiteHandler.HandleMozillaSuiteEvent(appleEvent, reply);
}







void AECoreClass::HandleGetURLSuiteEvent(const AppleEvent *appleEvent, AppleEvent *reply)
{
	mGetURLSuiteHandler.HandleGetURLSuiteEvent(appleEvent, reply);
}







void AECoreClass::HandleSpyglassSuiteEvent(const AppleEvent *appleEvent, AppleEvent *reply)
{
	mSpyglassSuiteHandler.HandleSpyglassSuiteEvent(appleEvent, reply);
}


#pragma mark -





pascal OSErr AECoreClass::PropertyTokenFromAnything(			DescType			desiredClass,
					 								const AEDesc*		containerToken,
					 								DescType			containerClass,
					 								DescType			keyForm,
				    									const AEDesc*		keyData,
					 								AEDesc*			resultToken,
					 								long 				refCon)
{
	AECoreClass*		coreClass = reinterpret_cast<AECoreClass *>(refCon);
	if (!coreClass) return paramErr;
	
	OSErr	err = noErr;
	
	try
	{
		coreClass->PropertyTokenFromList(desiredClass, containerToken, containerClass, keyForm, keyData, resultToken);
	}
	catch (OSErr catchErr)
	{
		err = catchErr;
	}
	catch (...)
	{
		err = paramErr;
	}
	
	return err;
}







pascal OSErr AECoreClass::AnythingFromAppAccessor(			DescType			desiredClass,
					 								const AEDesc*		containerToken,
					 								DescType			containerClass,
					 								DescType			keyForm,
				    									const AEDesc*		keyData,
					 								AEDesc*			resultToken,
					 								long 				refCon)
{
	AECoreClass*		coreClass = reinterpret_cast<AECoreClass *>(refCon);
	
	if (!coreClass)
		return paramErr;
	
	OSErr	err = noErr;
	
	try
	{
		coreClass->GetAnythingFromApp(desiredClass, containerToken, containerClass, keyForm, keyData, resultToken);
	}
	catch (OSErr catchErr)
	{
		err = catchErr;
	}
	catch (...)
	{
		err = paramErr;
	}
	
	return err;
}






pascal OSErr AECoreClass::CompareObjectsCallback(	DescType			comparisonOperator, 	
											const AEDesc *		object,				
											const AEDesc *		descriptorOrObject, 		
											Boolean *			result)
{
	OSErr	err = noErr;
	try
	{
		OSErr		err = noErr;
		StAEDesc		desc1;
		StAEDesc		desc2;

		
		AECoreClass::GetAECoreHandler()->ExtractData(object, &desc1);
			
		
		AECoreClass::GetAECoreHandler()->ExtractData(descriptorOrObject, &desc2);	

		
		
		
		if (desc1.descriptorType != desc2.descriptorType) 
		{
			StAEDesc		temp;
			
			
			
			
			err = AEDuplicateDesc(&desc2, &temp);
			ThrowIfOSErr(err);
			
			desc2.Clear();
			
			err = AECoerceDesc(&temp, desc1.descriptorType, &desc2);
			ThrowIfOSErr(err);
		}

		AEDispatchHandler*	handler = AECoreClass::GetDispatchHandlerForClass(desc1.descriptorType);
		if (handler)
			handler->CompareObjects(comparisonOperator, &desc1, &desc2, result);
		else
			*result = AEComparisons::TryPrimitiveComparison(comparisonOperator, &desc1, &desc2);
	}
	catch (OSErr catchErr)
	{
		err = catchErr;
	}
	catch (...)
	{
		err = paramErr;
	}
	
	return err;
}






pascal OSErr AECoreClass::CountObjectsCallback(		DescType 		 	desiredType,
											DescType 		 	containerClass,
											const AEDesc *		container,
							   				long *			result)
{
	AEDispatchHandler*	handler = AECoreClass::GetDispatchHandlerForClass(containerClass);
	if (!handler)
		return errAEEventNotHandled;
	
	OSErr	err = noErr;
	try
	{
		handler->CountObjects(				desiredType,
										containerClass,
										container,
										result);

	}
	catch (OSErr catchErr)
	{
		err = catchErr;
	}
	catch (...)
	{
		err = paramErr;
	}
	
	return err;
}

#pragma mark -






void AECoreClass::GetEventKeyDataParameter(const AppleEvent *appleEvent, DescType requestedType, AEDesc *data)
{
	StAEDesc	keyData;

	OSErr	err = AEGetKeyDesc(appleEvent, keyAEData, requestedType, &keyData);
	ThrowIfOSErr(err);
	
	ExtractData(&keyData, data);
}














void AECoreClass::ExtractData(const AEDesc *source, AEDesc *data)
{
	OSErr		err = noErr;
	StAEDesc		temp;
	DescType		dispatchClass;
		
	if ((source->descriptorType == typeNull) || (source->dataHandle == nil))
		ThrowIfOSErr(errAENoSuchObject);
	
	
	
	
	if (source->descriptorType == typeObjectSpecifier) 
	{
		err = AEResolve(source, kAEIDoMinimum, &temp);
	}
	else
	{
		err = AEDuplicateDesc(source, &temp);
	}
	ThrowIfOSErr(err);
	
	
	
	
	
	if (temp.descriptorType == typeProperty)
	{
		ConstAETokenDesc	tokenDesc(&temp);
		dispatchClass = tokenDesc.GetDispatchClass();
	}
	else
		dispatchClass = temp.descriptorType;
	
	
	
		
	AEDispatchHandler*	handler = GetDispatchHandler(dispatchClass);
	if (handler)
	{
		





		handler->GetDataFromListOrObject(&temp, nil, data);
	}
	else
	{
		err = AEDuplicateDesc(&temp, data);
		ThrowIfOSErr(err);
	}
}

#pragma mark -






void AECoreClass::RegisterClassHandler(DescType handlerClass, AEGenericClass* classHandler, Boolean isDuplicate )
{
	mDispatchTree.InsertHandler(handlerClass, classHandler, isDuplicate);
}






AEDispatchHandler* AECoreClass::GetDispatchHandler(DescType dispatchClass)
{
	return mDispatchTree.FindHandler(dispatchClass);
}









AEDispatchHandler* AECoreClass::GetDispatchHandlerForClass(DescType dispatchClass)
{
	if (!sAECoreHandler)
		return nil;
	
	return sAECoreHandler->GetDispatchHandler(dispatchClass);
}








void AECoreClass::InstallSuiteHandlers(Boolean suspendEvents)
{
	OSErr	err;

	err = ::AEInstallEventHandler(kCoreEventClass,  	typeWildCard, 
											suspendEvents ? mSuspendEventHandlerUPP : mRequiredSuiteHandlerUPP, 
											(long)this, 
											false);
	ThrowIfOSErr(err);
	
	err = ::AEInstallEventHandler(kAECoreSuite,  		typeWildCard, 
											suspendEvents ? mSuspendEventHandlerUPP : mStandardSuiteHandlerUPP, 
											(long)this, 
											false);
	ThrowIfOSErr(err);
	
	
	err = ::AEInstallEventHandler(AEMozillaSuiteHandler::kSuiteSignature,  	typeWildCard, 
											suspendEvents ? mSuspendEventHandlerUPP : mMozillaSuiteHandlerUPP, 
											(long)this,
											false);
	ThrowIfOSErr(err);

	
	err = ::AEInstallEventHandler(AEGetURLSuiteHandler::kSuiteSignature,  	typeWildCard, 
											suspendEvents ? mSuspendEventHandlerUPP : mGetURLSuiteHandlerUPP, 
											(long)this, 
											false);
	ThrowIfOSErr(err);
	
	
	err = ::AEInstallEventHandler(AESpyglassSuiteHandler::kSuiteSignature,  	typeWildCard, 
											suspendEvents ? mSuspendEventHandlerUPP : mSpyGlassSuiteHandlerUPP, 
											(long)this, 
											false);
	ThrowIfOSErr(err);
}






void AECoreClass::ResumeEventHandling(const AppleEvent *appleEvent, AppleEvent *reply, Boolean dispatchEvent)
{
	InstallSuiteHandlers(false);

	OSErr	err;

	
	err = ::AEResumeTheCurrentEvent(appleEvent, reply, (AEEventHandlerUPP)(dispatchEvent ? kAEUseStandardDispatch : kAENoDispatch), (long)this);
	ThrowIfOSErr(err);
}
