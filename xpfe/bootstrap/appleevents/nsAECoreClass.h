







































#ifndef __AECORECLASS__
#define __AECORECLASS__

#ifdef __cplusplus


#include <AEDataModel.h>
#include <AppleEvents.h>
#include <AEObjects.h>

#include "nsAEClassDispatcher.h"

#include "nsAEMozillaSuiteHandler.h"
#include "nsAEGetURLSuiteHandler.h"
#include "nsAESpyglassSuiteHandler.h"

class AEApplicationClass;
class AEDocumentClass;
class AEWindowClass;

class AECoreClass
{
public:
	
						AECoreClass(Boolean suspendEvents = false);	
						~AECoreClass();

	void					HandleCoreSuiteEvent(const AppleEvent *appleEvent, AppleEvent *reply);			
	void					HandleRequiredSuiteEvent(const AppleEvent *appleEvent, AppleEvent *reply);		

	void					HandleMozillaSuiteEvent(const AppleEvent *appleEvent, AppleEvent *reply);		
	void					HandleGetURLSuiteEvent(const AppleEvent *appleEvent, AppleEvent *reply);		
	void					HandleSpyglassSuiteEvent(const AppleEvent *appleEvent, AppleEvent *reply);		

	void					HandleCreateElementEvent(const AppleEvent *appleEvent, AppleEvent *reply);		
	
	void					HandleEventSuspend(const AppleEvent *appleEvent, AppleEvent *reply);
	void					ResumeEventHandling(const AppleEvent *appleEvent, AppleEvent *reply, Boolean dispatchEvent);
	
	
	static pascal OSErr		SuspendEventHandler(const AppleEvent *appleEvent, AppleEvent *reply, SInt32 refCon);
	static pascal OSErr		RequiredSuiteHandler(const AppleEvent *appleEvent, AppleEvent *reply, SInt32 refCon);
	static pascal OSErr		CoreSuiteHandler(const AppleEvent *appleEvent, AppleEvent *reply, SInt32 refCon);
	static pascal OSErr		CreateElementHandler(const AppleEvent *appleEvent, AppleEvent *reply, SInt32 refCon);

	
	static pascal OSErr		MozillaSuiteHandler(const AppleEvent *appleEvent, AppleEvent *reply, SInt32 refCon);

	
	static pascal OSErr		GetURLSuiteHandler(const AppleEvent *appleEvent, AppleEvent *reply, SInt32 refCon);

	
	static pascal OSErr		SpyglassSuiteHandler(const AppleEvent *appleEvent, AppleEvent *reply, SInt32 refCon);


	AEDispatchHandler*		GetDispatchHandler(DescType dispatchClass);
	
	
	void					GetSuspendedEvent(AppleEvent *appleEvent, AppleEvent *reply)
						{
							*appleEvent = mSuspendedEvent;
							*reply = mReplyToSuspendedEvent;
						}
						
protected:

	
	void					PropertyTokenFromList(			DescType			desiredClass,
					 								const AEDesc*		containerToken,
					 								DescType			containerClass,
					 								DescType			keyForm,
				    									const AEDesc*		keyData,
					 								AEDesc*			resultToken);
	
	void					GetAnythingFromApp(			DescType			desiredClass,
					 								const AEDesc*		containerToken,
					 								DescType			containerClass,
					 								DescType			keyForm,
				    									const AEDesc*		keyData,
					 								AEDesc*			resultToken);
	
	void					RegisterClassHandler(			DescType			handlerClass,
													AEGenericClass*	classHandler,
													Boolean			isDuplicate = false);


	void					InstallSuiteHandlers(				Boolean			suspendEvents);
	
public:

	void					GetEventKeyDataParameter(		const AppleEvent*	appleEvent,
													DescType			requestedType,
													AEDesc*			data);

	static pascal OSErr		PropertyTokenFromAnything(		DescType			desiredClass,
					 								const AEDesc*		containerToken,
					 								DescType			containerClass,
					 								DescType			keyForm,
				    									const AEDesc*		keyData,
					 								AEDesc*			resultToken,
					 								long 				refCon);

	static pascal OSErr		AnythingFromAppAccessor(		DescType			desiredClass,
					 								const AEDesc*		containerToken,
					 								DescType			containerClass,
					 								DescType			keyForm,
				    									const AEDesc*		keyData,
					 								AEDesc*			resultToken,
					 								long 				refCon);

	static pascal OSErr		CompareObjectsCallback(			DescType			comparisonOperator, 	
													const AEDesc *		object,				
													const AEDesc *		descriptorOrObject, 		
													Boolean *			result);


	static pascal OSErr		CountObjectsCallback(			DescType 		 	desiredType,
													DescType 		 	containerClass,
													const AEDesc *		container,
									   				long *			result);

	void					ExtractData(					const AEDesc*		source,
													AEDesc*			data);
	
	
	static AEDispatchHandler*	GetDispatchHandlerForClass(	DescType			dispatchClass);
	
protected:
	
	AEDispatchTree			mDispatchTree;				

	AEEventHandlerUPP		mSuspendEventHandlerUPP;
	
	AEEventHandlerUPP		mStandardSuiteHandlerUPP;
	AEEventHandlerUPP		mRequiredSuiteHandlerUPP;
	AEEventHandlerUPP		mCreateElementHandlerUPP;

	AEEventHandlerUPP		mMozillaSuiteHandlerUPP;
	AEEventHandlerUPP		mGetURLSuiteHandlerUPP;
	AEEventHandlerUPP		mSpyGlassSuiteHandlerUPP;

        AEMozillaSuiteHandler      mMozillaSuiteHandler;
        AEGetURLSuiteHandler      mGetURLSuiteHandler;
        AESpyglassSuiteHandler   mSpyglassSuiteHandler;
        
	OSLAccessorUPP		mPropertyFromListAccessor;
	OSLAccessorUPP		mAnythingFromAppAccessor;

	OSLCountUPP			mCountItemsCallback;
	OSLCompareUPP		mCompareItemsCallback;

private:

	AppleEvent			mSuspendedEvent;
	AppleEvent			mReplyToSuspendedEvent;
	
public:

	static AECoreClass*		GetAECoreHandler() { return sAECoreHandler; }
	static AECoreClass*		sAECoreHandler;

};

#endif	

#endif 

