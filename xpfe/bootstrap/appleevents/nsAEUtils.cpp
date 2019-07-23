






































#include <string.h>


#include "nsAETokens.h"
#include "nsAEUtils.h"

static OSErr AECoerceDescData(const AEDesc *theAEDesc, DescType typeCode, void *dataPtr, Size maximumSize);







OSErr CreateAliasAEDesc(AliasHandle theAlias, AEDesc *ioDesc)
{
	char		state = HGetState((Handle)theAlias);
	OSErr	err;

	HLock((Handle)theAlias);
	err = AECreateDesc(typeAlias, *theAlias, GetHandleSize((Handle)theAlias), ioDesc);
	HSetState((Handle)theAlias, state);
	return err;
}







OSErr GetTextFromAEDesc(const AEDesc *inDesc, Handle *outTextHandle)
{
	Handle	textHandle = nil;
	Size		textLength;
	OSErr	err;

	textLength = AEGetDescDataSize(inDesc);

	err = MyNewHandle(textLength, &textHandle);
	if (err != noErr) return err;
	
	MyHLock(textHandle);
	err = AECoerceDescData(inDesc, typeChar, *textHandle, textLength);
	MyHUnlock(textHandle);
	
	if (err != noErr)
		goto exit;
	
	*outTextHandle = textHandle;
	return noErr;
	
exit:
	MyDisposeHandle(textHandle);
	return err;
}


#if !TARGET_CARBON








OSErr AEGetDescData(const AEDesc *theAEDesc, void *dataPtr, Size maximumSize)
{
  OSErr err = noErr;
  
	if (theAEDesc->dataHandle)
	{
		Size dataLength = GetHandleSize(theAEDesc->dataHandle);
		BlockMoveData(*theAEDesc->dataHandle, dataPtr, Min(dataLength, maximumSize));
	}
	else
		err = paramErr;
		
	return err;
}






Size AEGetDescDataSize(const AEDesc *theAEDesc)
{
	Size		dataSize = 0;
	
	if (theAEDesc->dataHandle)
		dataSize = GetHandleSize(theAEDesc->dataHandle);
	
	return dataSize;
}






OSErr AEReplaceDescData(DescType typeCode, const void *dataPtr, Size dataSize, AEDesc* theAEDesc)
{
	AEDisposeDesc(theAEDesc);
	return AECreateDesc(typeCode, dataPtr, dataSize, theAEDesc);
}

#endif 

static OSErr AECoerceDescData(const AEDesc *theAEDesc, DescType typeCode, void *dataPtr, Size maximumSize)
{
	OSErr err;
	
	if (theAEDesc->descriptorType != typeCode)
	{
		AEDesc	coercedDesc = { typeNull, nil };
		err = AECoerceDesc(theAEDesc, typeCode, &coercedDesc);
		if (err != noErr) return err;
		
		err = AEGetDescData(&coercedDesc, dataPtr, maximumSize);
		AEDisposeDesc(&coercedDesc);
		return err;
	}
	else
	{
	    return AEGetDescData(theAEDesc, dataPtr, maximumSize);
	}
}

#pragma mark -







OSErr CreateThreadAEInfo(const AppleEvent *event, AppleEvent *reply, TThreadAEInfoPtr *outThreadAEInfo)
{
	TThreadAEInfo	*threadAEInfo = nil;
	OSErr		err;
	
	err = MyNewBlockClear(sizeof(TThreadAEInfo), (void**)&threadAEInfo);
	if (err != noErr) return err;
	
	threadAEInfo->mAppleEvent = *event;
	threadAEInfo->mReply = *reply;
	threadAEInfo->mGotEventData = event && reply;
	threadAEInfo->mSuspendCount = nil;
	
	*outThreadAEInfo = threadAEInfo;
	return noErr;

exit:
	MyDisposeBlock(threadAEInfo);
	return err;
}







void DisposeThreadAEInfo(TThreadAEInfo *threadAEInfo)
{
	AE_ASSERT(threadAEInfo && threadAEInfo->mSuspendCount == 0, "Bad suspend count");
	MyDisposeBlock(threadAEInfo);
}







OSErr SuspendThreadAE(TThreadAEInfo *threadAEInfo)
{
	if (threadAEInfo == nil) return noErr;
	if (!threadAEInfo->mGotEventData) return noErr;
	
	if (threadAEInfo->mSuspendCount == 0)
	{
		OSErr	err = AESuspendTheCurrentEvent(&threadAEInfo->mAppleEvent);
		if (err != noErr) return err;
	}
	
	++ threadAEInfo->mSuspendCount;
	return noErr;
}








static OSErr AddErrorCodeToReply(TThreadAEInfo *threadAEInfo, OSErr threadError)
{
	long		errorValue = threadError;
	
	if (threadError == noErr) return noErr;
	
	return AEPutParamPtr(&threadAEInfo->mReply, keyErrorNumber, typeLongInteger,  (Ptr)&errorValue, sizeof(long));
}







OSErr ResumeThreadAE(TThreadAEInfo *threadAEInfo, OSErr threadError)
{
	if (threadAEInfo == nil) return noErr;
	if (!threadAEInfo->mGotEventData) return noErr;

	-- threadAEInfo->mSuspendCount;
	
	AddErrorCodeToReply(threadAEInfo, threadError);
	
	if (threadAEInfo->mSuspendCount == 0)
		return AEResumeTheCurrentEvent(&threadAEInfo->mAppleEvent, &threadAEInfo->mReply, (AEEventHandlerUPP)kAENoDispatch, 0);

	return noErr;
}


#pragma mark -








StAEDesc::StAEDesc(const StAEDesc& rhs)
{
	ThrowIfOSErr(AEDuplicateDesc(&rhs, this));
}








StAEDesc& StAEDesc::operator= (const StAEDesc& rhs)
{
	ThrowIfOSErr(AEDuplicateDesc(&rhs, this));
	return *this;
}








Boolean StAEDesc::GetBoolean()
{
    Boolean result = false;
    OSErr err = ::AECoerceDescData(this, typeBoolean, &result, sizeof(result));
    if (err != noErr)
        ThrowOSErr(errAECoercionFail);
    return result;
}

SInt16 StAEDesc::GetShort()
{
    SInt16 result = 0;
    OSErr err = ::AECoerceDescData(this, typeShortInteger, &result, sizeof(result));
    if (err != noErr)
        ThrowOSErr(errAECoercionFail);
    return result;
}

SInt32 StAEDesc::GetLong()
{
    SInt32 result = 0;
    OSErr err = ::AECoerceDescData(this, typeLongInteger, &result, sizeof(result));
    if (err != noErr)
        ThrowOSErr(errAECoercionFail);
    return result;
}

DescType StAEDesc::GetEnumType()
{
    DescType result = typeNull;
    OSErr err = ::AECoerceDescData(this, typeEnumeration, &result, sizeof(result));
    if (err != noErr)
        ThrowOSErr(errAECoercionFail);
    return result;
}

void StAEDesc::GetRect(Rect& outData)
{
    OSErr err = ::AECoerceDescData(this, typeQDRectangle, &outData, sizeof(Rect));
    if (err != noErr)
        ThrowOSErr(errAECoercionFail);
}


void StAEDesc::GetRGBColor(RGBColor& outData)
{
    OSErr err = ::AECoerceDescData(this, typeRGBColor, &outData, sizeof(RGBColor));
    if (err != noErr)
        ThrowOSErr(errAECoercionFail);
}

void StAEDesc::GetLongDateTime(LongDateTime& outDateTime)
{
    OSErr err = ::AECoerceDescData(this, typeLongDateTime, &outDateTime, sizeof(LongDateTime));
    if (err != noErr)
        ThrowOSErr(errAECoercionFail);
}

void StAEDesc::GetFileSpec(FSSpec &outFileSpec)
{
    OSErr err = ::AECoerceDescData(this, typeFSS, &outFileSpec, sizeof(FSSpec));
    if (err != noErr)
        ThrowOSErr(errAECoercionFail);
}

void StAEDesc::GetCString(char *outString, short maxLen)
{
	if (descriptorType == typeChar)
	{
		long dataSize = GetDataSize();
		dataSize = Min(dataSize, maxLen-1);
		if (AEGetDescData(this, outString, dataSize) == noErr)
    		outString[dataSize] = '\0';
	}
	else
	{
		StAEDesc tempDesc;
		if (::AECoerceDesc(this, typeChar, &tempDesc) == noErr)
		{
			long dataSize = tempDesc.GetDataSize();
    		dataSize = Min(dataSize, maxLen-1);
    		if (AEGetDescData(&tempDesc, outString, dataSize) == noErr)
        		outString[dataSize] = '\0';
		}
		else
			ThrowOSErr(errAECoercionFail);
	}
}

void StAEDesc::GetPString(Str255 outString)
{
	if (descriptorType == typeChar)
	{
		long stringLen = GetDataSize();
		if (stringLen > 255)
			stringLen = 255;
		AEGetDescData(this, outString+1, stringLen);
		outString[0] = stringLen;
	}
	else
	{
		StAEDesc	tempDesc;
		if (::AECoerceDesc(this, typeChar, &tempDesc) == noErr)
		{
			long stringLen = tempDesc.GetDataSize();
			if (stringLen > 255)
				stringLen = 255;
			AEGetDescData(&tempDesc, outString+1, stringLen);
			outString[0] = stringLen;
		}
		else
			ThrowOSErr(errAECoercionFail);
	}
}

Handle StAEDesc::GetTextHandle()
{
    Handle data = nil;
    
	if (descriptorType == typeChar)
	{
	    Size dataSize = GetDataSize();
	    data = ::NewHandle(dataSize);
	    if (data == NULL)
	        ThrowOSErr(memFullErr);
        ::HLock(data);
        ::AEGetDescData(this, *data, dataSize);
        ::HUnlock(data);
	}
	else
	{
    	StAEDesc tempDesc;
		if (::AECoerceDesc(this, typeChar, &tempDesc) == noErr)
		    data = tempDesc.GetTextHandle();
		else
		    ThrowOSErr(errAECoercionFail);
	}
	
	return data;
}


#pragma mark -







OSErr AEListUtils::GetFirstNonListToken(const AEDesc *token, AEDesc *result)
{
	OSErr 		err = noErr;
	AEDesc 		tempToken = { typeNull, nil };
	AEKeyword	keyword;
	long			numItems;
	long 			itemNum;
	
	if (result->descriptorType == typeNull)
	{
		if (TokenContainsTokenList(token) == false)
		{
			err = AEDuplicateDesc(token, result);
		}
		else
		{
			err = AECountItems(token, &numItems);
			
			for (itemNum = 1; itemNum <= numItems; itemNum++)
			{
				err = AEGetNthDesc((AEDescList *)token, itemNum, typeWildCard, &keyword, &tempToken);
				if (err != noErr)
					goto CleanUp;
					
				err = GetFirstNonListToken(&tempToken, result);				
				if ((err != noErr) || (result->descriptorType != typeNull))
					break;
					
				AEDisposeDesc(&tempToken);
			}
		}
	}
	
CleanUp:
	if (err != noErr)
		AEDisposeDesc(result);
		
	AEDisposeDesc(&tempToken);
	return err;
}





OSErr AEListUtils::FlattenAEList(AEDescList *deepList, AEDescList *flatList)
{
	OSErr		err = noErr;
	AEDesc		item	= {typeNull, nil};
	AEKeyword	keyword;
	long			itemNum;
	long			numItems;

	err = AECountItems(deepList, &numItems);
	if (err != noErr)
		goto CleanUp;
	
	for (itemNum = 1; itemNum <= numItems; itemNum++)
	{
		err = AEGetNthDesc(deepList, itemNum, typeWildCard, &keyword, &item);
		if (err != noErr)
			goto CleanUp;
			
		if (item.descriptorType == typeAEList)
			err = FlattenAEList(&item, flatList);
		else
			err = AEPutDesc(flatList, 0L, &item);
		
		if (err != noErr)
			goto CleanUp;
			
		AEDisposeDesc(&item);
	}

CleanUp:
	if (err != noErr)
		AEDisposeDesc(flatList);

	AEDisposeDesc(&item);
	
	return err;
}


#pragma mark -





StEventSuspender::StEventSuspender(const AppleEvent *appleEvent, AppleEvent *reply, Boolean deleteData)
:	mThreadAEInfo(nil)
,	mDeleteData(deleteData)
{
	ThrowIfOSErr(CreateThreadAEInfo(appleEvent, reply, &mThreadAEInfo));
}






StEventSuspender::~StEventSuspender()
{
	if (mDeleteData)
		DisposeThreadAEInfo(mThreadAEInfo);
}





void StEventSuspender::SuspendEvent()
{
	ThrowIfOSErr(SuspendThreadAE(mThreadAEInfo));
}






void StEventSuspender::ResumeEvent()
{
	ThrowIfOSErr(ResumeThreadAE(mThreadAEInfo, noErr));
}



#pragma mark -






StHandleHolder::StHandleHolder(Handle inHandle)
:	mHandle(inHandle)
,	mLockCount(0)
{
}






StHandleHolder::~StHandleHolder()
{
	if (mHandle)
		DisposeHandle(mHandle);
}






StHandleHolder& StHandleHolder::operator=(Handle rhs)
{
	AE_ASSERT(mLockCount == 0, "Bad lock count");
	mLockCount = 0;
	
	if (mHandle)
		DisposeHandle(mHandle);
		
	mHandle = rhs;
	return *this;
}





void StHandleHolder::Lock()
{
	ThrowIfNil(mHandle);
	if (++mLockCount > 1) return;
	mOldHandleState = HGetState(mHandle);
	HLock(mHandle);
}





void StHandleHolder::Unlock()
{
	ThrowIfNil(mHandle);
	AE_ASSERT(mLockCount > 0, "Bad lock count");
	if (--mLockCount == 0)
		HSetState(mHandle, mOldHandleState);
}


#pragma mark -






AEListIterator::AEListIterator(AEDesc *token)
:	mNumItems(0)
,	mCurItem(0)
,	mIsListDesc(false)
{
	ThrowIfNil(token);
	mListToken = *token;
	mIsListDesc = AEListUtils::TokenContainsTokenList(&mListToken);
	if (mIsListDesc)
	{
		ThrowIfOSErr(::AECountItems(token, &mNumItems));
		mCurItem = 1;
	}
	else
	{
		mCurItem = 0;
		mNumItems = 1;
	}
}






Boolean AEListIterator::Next(AEDesc* outItemData)
{
	if (mIsListDesc)
	{
		AEKeyword	keyword;
		
		if (mCurItem == 0 || mCurItem > mNumItems)
			return false;
		
		ThrowIfOSErr(::AEGetNthDesc(&mListToken, mCurItem, typeWildCard, &keyword, outItemData));
		
		
		AE_ASSERT(!AEListUtils::TokenContainsTokenList(outItemData), "Nested list found");
	}
	else
	{
		if (mCurItem > 0)
			return false;
		
		ThrowIfOSErr(::AEDuplicateDesc(&mListToken, outItemData));
	}
	
	mCurItem ++;
	return true;
}



#pragma mark -







OSErr  CheckForUnusedParameters(const AppleEvent* appleEvent)
{
	OSErr		err 		= noErr;
	
	DescType	actualType	= typeNull;
	Size		actualSize	= 0L;
	
	err = AEGetAttributePtr(appleEvent, 
									keyMissedKeywordAttr, 
									typeWildCard,
									&actualType, 
									nil, 
									0, 
									&actualSize);
									
	if (err == errAEDescNotFound)
		err = noErr;
	else
		err = errAEParamMissed;
		
	return err;
}






OSErr  PutReplyErrorNumber(AppleEvent* reply, long errorNumber)
{
	OSErr err = noErr;
	
	if (reply->dataHandle != nil && errorNumber != noErr)
	{
		err = AEPutParamPtr(reply, 
									 keyErrorNumber, 
									 typeLongInteger, 
									 (Ptr)&errorNumber, 
									 sizeof(long));
	}
	return err;
}






OSErr  PutReplyErrorMessage(AppleEvent* reply, char *message)
{
	OSErr err = noErr;
	
	if (reply->dataHandle != nil && message != NULL)
	{
		err = AEPutParamPtr(reply, 
									 keyErrorString, 
									 typeChar, 
									 (Ptr)message, 
									 strlen(message));
	}
	return err;
}











OSErr GetObjectClassFromAppleEvent(const AppleEvent *appleEvent, DescType *objectClass)
{
	OSErr	err     = noErr;
	OSType	typeCode;					
	long		actualSize;
	
	

	err = AEGetParamPtr(appleEvent, 
								 keyAEObjectClass, 
								 typeType, 
								 &typeCode,
								 (Ptr)objectClass, 
								 sizeof(DescType), 
								 &actualSize);
								 
	if (typeCode != typeType) 
		err = errAECoercionFail;
	
	return err;
}

#pragma mark -







OSErr DescToPString(const AEDesc* desc, Str255 aPString, short maxLength)
{
	if (desc->descriptorType == typeChar)
	{
		long stringLen = AEGetDescDataSize(desc);
		if (stringLen > maxLength)
			stringLen = maxLength;
		AEGetDescData(desc, aPString+1, stringLen);
		aPString[0] = stringLen;
	}
	else
	{
    	StAEDesc tempDesc;
		if (AECoerceDesc(desc, typeChar, &tempDesc) == noErr) {
    		long stringLen = tempDesc.GetDataSize();
    		if (stringLen > maxLength)
    			stringLen = maxLength;
    		AEGetDescData(&tempDesc, aPString+1, stringLen);
    		aPString[0] = stringLen;
		} else
			return errAECoercionFail;
	}
	return noErr;
}








OSErr DescToCString(const AEDesc* desc, CStr255 aCString, short maxLength)
{
	if (desc->descriptorType == typeChar)
	{
	    long stringLen = AEGetDescDataSize(desc);
	    if (stringLen >= maxLength)
	        stringLen = maxLength - 1;
		if (AEGetDescData(desc, aCString, stringLen) == noErr)
    		aCString[stringLen] = '\0';
    	else
			return errAECoercionFail;
	}
	else
	{
    	StAEDesc tempDesc;
		if (AECoerceDesc(desc, typeChar, &tempDesc) == noErr) {
    	    long stringLen = AEGetDescDataSize(&tempDesc);
    	    if (stringLen >= maxLength)
    	        stringLen = maxLength - 1;
    		if (AEGetDescData(&tempDesc, aCString, stringLen) == noErr)
        		aCString[stringLen] = '\0';
        	else
    			return errAECoercionFail;
		} else
			return errAECoercionFail;
	}
	return noErr;
}





OSErr DescToDescType(const AEDesc *desc, DescType *descType)
{
	if (AEGetDescDataSize(desc) == sizeof(DescType))
	    return AEGetDescData(desc, descType, sizeof(DescType));
	else
		return errAECoercionFail;
}





OSErr DescToBoolean(const AEDesc* desc, Boolean* aBoolean)
{
    return AECoerceDescData(desc, typeBoolean, aBoolean, sizeof(Boolean));
}





OSErr DescToFixed(const  AEDesc* desc, Fixed* aFixed)
{
    return AECoerceDescData(desc, typeFixed, aFixed, sizeof(Fixed));
}





OSErr DescToFloat(const  AEDesc* desc, float* aFloat)
{
    return AECoerceDescData(desc, typeFloat, aFloat, sizeof(float));
}





OSErr DescToLong(const AEDesc* desc, long* aLong)
{
    return AECoerceDescData(desc, typeLongInteger, aLong, sizeof(long));
}





OSErr DescToRGBColor(const  AEDesc* desc, RGBColor* aRGBColor)
{
    return AECoerceDescData(desc, typeRGBColor, aRGBColor, sizeof(RGBColor));
}





OSErr DescToShort(const  AEDesc* desc, short* aShort)
{
    return AECoerceDescData(desc, typeShortInteger, aShort, sizeof(short));
}





OSErr DescToTextHandle(const AEDesc* desc, Handle *text)
{
    Handle data = nil;
    
	if (desc->descriptorType == typeChar)
	{
	    Size dataSize = ::AEGetDescDataSize(desc);
	    data = ::NewHandle(dataSize);
	    if (data == NULL)
	        return memFullErr;
        ::HLock(data);
        ::AEGetDescData(desc, *data, dataSize);
        ::HUnlock(data);
	}
	else
	{
    	StAEDesc tempDesc;
		if (::AECoerceDesc(desc, typeChar, &tempDesc) == noErr)
		    data = tempDesc.GetTextHandle();
		else
		    return errAECoercionFail;
	}
	*text = data;
	return noErr;
}





OSErr DescToRect(const  AEDesc* desc, Rect* aRect)
{
    return AECoerceDescData(desc, typeRectangle, aRect, sizeof(Rect));
}





OSErr DescToPoint(const  AEDesc* desc, Point* aPoint)
{
    return AECoerceDescData(desc, typeQDPoint, aPoint, sizeof(Point));
}

#pragma mark -







OSErr NormalizeAbsoluteIndex(const AEDesc *keyData, long *index, long maxIndex, Boolean *isAllItems)
{
	OSErr err = noErr;
	
	*isAllItems = false;								
	
	
	
	switch (keyData->descriptorType)
	{
		case typeLongInteger:						
			if (DescToLong(keyData, index) != noErr)
				return errAECoercionFail;

			if (*index < 0)							
				*index = maxIndex + *index + 1;
			break;
			
	   case typeAbsoluteOrdinal:										
	   		DescType		ordinalDesc;
			if (DescToDescType((AEDesc*)keyData, &ordinalDesc) != noErr)
				ThrowOSErr(errAECoercionFail);
			
			switch (ordinalDesc)
			{
			   	case kAEFirst:
			   		*index = 1;
					break;
					
				case kAEMiddle:
					*index = (maxIndex >> 1) + (maxIndex % 2);
					break;
						
			   	case kAELast:
			   		*index = maxIndex;
					break;
						
			   	case kAEAny:
					*index = (TickCount() % maxIndex) + 1;		
					break;
						
			   	case kAEAll:
			   		*index = 1;
			   		*isAllItems = true;
					break;
			}
			break;

		default:
			return errAEWrongDataType;
			break;
	}

	
	if ((*index < 1) || (*index > maxIndex))					
	{
		err = errAEIllegalIndex;
	}

	return err;
}





OSErr ProcessFormRange(AEDesc *keyData, AEDesc *start, AEDesc *stop)
{
	OSErr 		err = noErr;
	StAEDesc 		rangeRecord;
	StAEDesc		ospec;
	
	
	
 	err = AECoerceDesc(keyData, typeAERecord, &rangeRecord);
 	if (err != noErr)
 		return err;
	 
	
	err = AEGetKeyDesc(&rangeRecord, keyAERangeStart, typeWildCard, &ospec);
 	if (err == noErr && ospec.descriptorType == typeObjectSpecifier)
 		err = AEResolve(&ospec, kAEIDoMinimum, start);
 		
 	if (err != noErr)
 		return err;
 		
	ospec.Clear();
		
	
	
	err = AEGetKeyDesc(&rangeRecord, keyAERangeStop, typeWildCard, &ospec);
  	if (err == noErr && ospec.descriptorType == typeObjectSpecifier)
 		err = AEResolve(&ospec, kAEIDoMinimum, stop);

	return err;
}
