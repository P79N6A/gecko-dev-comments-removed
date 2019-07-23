







































#ifndef __AEUTILS__
#define __AEUTILS__

#include <MacTypes.h>
#include <AppleEvents.h>
#include <AERegistry.h>
#include <ASRegistry.h>
#include <AEObjects.h>
#include <FinderRegistry.h>

#include <Files.h>
#include <Errors.h>
#include <Aliases.h>
#include <MacWindows.h>

#include "nsAEDefs.h"
#include "nsMacUtils.h"


#define ThrowIfNil(a)      			do { if (a == nil) { throw((OSErr)memFullErr); } } while (0)
#define ThrowErrIfNil(a, err)		do { if (a == nil) { OSErr theErr = (err); throw((OSErr)theErr); } } while (0)
#define ThrowErrIfTrue(a, err)		do { if (a) { OSErr theErr = (err); throw((OSErr)theErr); } } while (0)
#define ThrowIfOSErr(err)      		do { OSErr theErr = (err); if (theErr != noErr) { throw((OSErr)theErr); } } while (0)
#define ThrowOSErr(err)        		do { throw((OSErr)err); } while (0)
#define ThrowNoErr()				do { throw((OSErr)noErr); } while (0)


#define pObjectType				'pObT'


#define pTitle					'pTit'
#define pIsModeless				'pNMo'
#define pIsMovableModal      		'pMMo'
#define pIsSuspended			'pSus'
#define pIsPalette				'pPal'
#define pIsDialog				'pDlg'
#define pLocation				'pLcn'   // the upper left corner of the object's bounding box



#define pFreeMemory			'pMem'
#define pTicks					'pTic'





typedef struct TThreadAEInfo
{
	AppleEvent		mAppleEvent;
	AppleEvent		mReply;
	SInt32			mSuspendCount;			
	Boolean			mGotEventData;
} TThreadAEInfo, *TThreadAEInfoPtr;




#ifdef __cplusplus
extern "C" {
#endif

OSErr CreateAliasAEDesc(AliasHandle theAlias, AEDesc *ioDesc);
OSErr GetTextFromAEDesc(const AEDesc *inDesc, Handle *outTextHandle);

OSErr CreateThreadAEInfo(const AppleEvent *event, AppleEvent *reply, TThreadAEInfoPtr *outThreadAEInfo);
void DisposeThreadAEInfo(TThreadAEInfo *threadAEInfo);

OSErr SuspendThreadAE(TThreadAEInfo *threadAEInfo);
OSErr ResumeThreadAE(TThreadAEInfo *threadAEInfo, OSErr threadError);

#if !TARGET_CARBON

OSErr AEGetDescData(const AEDesc *theAEDesc,  void * dataPtr, Size maximumSize);
Size AEGetDescDataSize(const AEDesc *theAEDesc);
OSErr AEReplaceDescData(DescType typeCode, const void *dataPtr, Size dataSize, AEDesc* theAEDesc);

#endif 

#ifdef __cplusplus
}
#endif




class StAEDesc: public AEDesc
{
public:
					StAEDesc()
					{
						descriptorType = typeNull;
						dataHandle = nil;
					}
					
					StAEDesc(const StAEDesc& rhs);				
					
					~StAEDesc()
					{
						::AEDisposeDesc(this);
					}

					void	Clear()
					{
						::AEDisposeDesc(this);
						descriptorType = typeNull;
						dataHandle = nil;
					}
					
					void CheckDataType(DescType expectedType)
					{
						if (descriptorType != expectedType)
							ThrowOSErr(errAEWrongDataType);
					}
					
	StAEDesc& 		operator= (const StAEDesc&rhs);		

	Size			GetDataSize()
	                {
                        return AEGetDescDataSize(this);
	                }
		
	Boolean			GetBoolean();
	SInt16			GetShort();
	SInt32			GetLong();
	DescType		GetEnumType();
	
	void			GetRect(Rect& outRect);
	void			GetRGBColor(RGBColor& outColor);
	void			GetLongDateTime(LongDateTime& outDateTime);
	void			GetFileSpec(FSSpec &outFileSpec);
	void			GetCString(char *outString, short maxLen);
	void			GetPString(Str255 outString);
	
	Handle			GetTextHandle();
	
};


class StEventSuspender
{
public:

					
					StEventSuspender(const AppleEvent *appleEvent, AppleEvent *reply, Boolean deleteData = true);
					~StEventSuspender();

	void				SetDeleteData(Boolean deleteData)	{ mDeleteData = deleteData; }
	void				SuspendEvent();
	void				ResumeEvent();
	
	TThreadAEInfoPtr	GetThreadAEInfo()	{ return mThreadAEInfo; }
	
protected:

	TThreadAEInfoPtr	mThreadAEInfo;
	Boolean			mDeleteData;
};

class StHandleHolder
{
public:
					StHandleHolder(Handle inHandle = nil);		
					~StHandleHolder();
					
	StHandleHolder&	operator=(Handle rhs);
					
	void				Lock();
	void				Unlock();
	
	Handle			GetHandle()		{ return mHandle; }
	Size				GetHandleSize()		{ return (mHandle) ? ::GetHandleSize(mHandle) : 0; } 
	Ptr				GetPtr()			{ return (mHandle) ? *mHandle : nil; }
	
	class getter
	{
	public:
					getter(StHandleHolder& handleHolder)
					:	mHandleHolder(handleHolder)
					,	mHandle(nil)
					{
					}
					
					~getter()
					{
						mHandleHolder = mHandle;
					}
					
					operator Handle*()
					{
						return &mHandle;
					}
	private:
		StHandleHolder&	mHandleHolder;
		Handle			mHandle;
	};

	
	getter			operator &()
					{
						return getter(*this);
					}
	
	getter			AssignHandle()
					{
						return getter(*this);
					}
	
protected:
	
	Handle			mHandle;
	SInt32			mLockCount;
	UInt8			mOldHandleState;
};


class AEListUtils
{
public:

	static Boolean	TokenContainsTokenList(const AEDesc *token) { return (token->descriptorType == typeAEList); }
	static OSErr	GetFirstNonListToken(const AEDesc *token, AEDesc *result);
	static OSErr 	FlattenAEList(AEDescList *deepList, AEDescList *flatList);
};




class AEListIterator
{
public:
				AEListIterator(AEDesc *token);
				~AEListIterator() {}
				
	Boolean		Next(AEDesc* outItemData);			
	SInt32		GetNumItems()  { return mNumItems; }
	
protected:
	AEDesc		mListToken;		
	SInt32		mNumItems;
	SInt32		mCurItem;
	Boolean		mIsListDesc;

};


OSErr	DescToPString(const AEDesc* desc, Str255 aPString, short maxLength);
OSErr 	DescToCString(const AEDesc* desc, CStr255 aPString, short maxLength);
OSErr	DescToDescType(const AEDesc *desc, DescType *descType);
OSErr	DescToBoolean(const AEDesc* desc, Boolean* aBoolean);
OSErr	DescToFixed(const  AEDesc* desc, Fixed* aFixed);
OSErr 	DescToFloat(const  AEDesc* desc, float* aFloat);
OSErr 	DescToLong(const  AEDesc* desc, long* aLong);
OSErr 	DescToRGBColor(const  AEDesc* desc, RGBColor* aRGBColor);
OSErr 	DescToShort(const  AEDesc* desc, short* aShort);
OSErr 	DescToTextHandle(const AEDesc* desc, Handle *text);
OSErr 	DescToRect(const  AEDesc* desc, Rect* aRect);
OSErr 	DescToPoint(const  AEDesc* desc, Point* aPoint);

OSErr	CheckForUnusedParameters(const AppleEvent* appleEvent);
OSErr	PutReplyErrorNumber(AppleEvent* reply, long errorNumber);
OSErr	PutReplyErrorMessage(AppleEvent* reply, char *message);
OSErr	GetObjectClassFromAppleEvent(const AppleEvent *appleEvent, DescType *objectClass);
OSErr	NormalizeAbsoluteIndex(const AEDesc *keyData, long *index, long maxIndex, Boolean *isAllItems);
OSErr 	ProcessFormRange(AEDesc *keyData, AEDesc *start, AEDesc *stop);


#endif 
