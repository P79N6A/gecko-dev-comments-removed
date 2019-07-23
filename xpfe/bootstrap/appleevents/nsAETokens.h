






































#ifndef __AETOKENS__
#define __AETOKENS__

#include "nsAEUtils.h"







struct CoreTokenRecord
{
	DescType				dispatchClass;		
	DescType				objectClass;		
	DescType				propertyCode;		
	long					documentID;
	TAEListIndex			elementNumber;
	WindowPtr			window;			
	
	CoreTokenRecord()
	:	dispatchClass(typeNull)
	,	objectClass(typeNull)
	,	propertyCode(typeNull)
	,	documentID(0)
	,	elementNumber(0)
	,	window(nil)
	{
	}
};


typedef struct CoreTokenRecord CoreTokenRecord, *CoreTokenPtr, **CoreTokenHandle;







class ConstAETokenDesc
{
public:
						ConstAETokenDesc(const AEDesc* token);


	DescType			GetDispatchClass() const;
	DescType 			GetObjectClass() const;
	Boolean				UsePropertyCode() const;
	DescType			GetPropertyCode() const;
		
	long				GetDocumentID() const;
	WindowPtr			GetWindowPtr() const;
	TAEListIndex		GetElementNumber() const;

protected:

	CoreTokenRecord     mTokenRecord;
	Boolean				mTokenWasNull;		
};







class AETokenDesc : public ConstAETokenDesc
{
public:
						AETokenDesc(AEDesc* token);
						~AETokenDesc();
	
	void				SetDispatchClass(DescType dispatchClass);
	void				SetObjectClass(DescType objectClass);
	void				SetPropertyCode(DescType propertyCode);
	void				SetElementNumber(TAEListIndex number);
	void				SetWindow(WindowPtr wind);

	void				UpdateDesc();			

	CoreTokenRecord&	GetTokenRecord()	{ return mTokenRecord; }
	
protected:
	
	AEDesc*				mTokenDesc;
	
};

#endif 
