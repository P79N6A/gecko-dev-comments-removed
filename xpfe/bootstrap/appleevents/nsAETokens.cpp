







































#include "nsAETokens.h"


ConstAETokenDesc::ConstAETokenDesc(const AEDesc* token)
{
	mTokenWasNull = (token->descriptorType == typeNull);
	
	if (!mTokenWasNull)
	{
		if (::AEGetDescDataSize(token) != sizeof(CoreTokenRecord))
			ThrowOSErr(paramErr);			

		ThrowIfOSErr(::AEGetDescData(token, &mTokenRecord, sizeof(CoreTokenRecord)));
	}
}




DescType ConstAETokenDesc::GetDispatchClass() const
{
	ThrowErrIfTrue(mTokenWasNull, paramErr);
	return mTokenRecord.dispatchClass;
}



DescType ConstAETokenDesc::GetObjectClass() const
{
	ThrowErrIfTrue(mTokenWasNull, paramErr);
	return mTokenRecord.objectClass;
}






Boolean ConstAETokenDesc::UsePropertyCode() const
{
	ThrowErrIfTrue(mTokenWasNull, paramErr);
	return (mTokenRecord.propertyCode != typeNull);
}



DescType ConstAETokenDesc::GetPropertyCode() const
{
	ThrowErrIfTrue(mTokenWasNull, paramErr);
	return mTokenRecord.propertyCode;
}



long ConstAETokenDesc::GetDocumentID() const
{
	ThrowErrIfTrue(mTokenWasNull, paramErr);
	return mTokenRecord.documentID;
}



WindowPtr ConstAETokenDesc::GetWindowPtr() const
{
	ThrowErrIfTrue(mTokenWasNull, paramErr);
	return mTokenRecord.window;
}




TAEListIndex ConstAETokenDesc::GetElementNumber() const
{
	ThrowErrIfTrue(mTokenWasNull, paramErr);
	return mTokenRecord.elementNumber;
}



#pragma mark -


AETokenDesc::AETokenDesc(AEDesc* token)
:	ConstAETokenDesc(token)
,	mTokenDesc(token)
{
	if (mTokenWasNull)			
		ThrowOSErr(paramErr);
}


AETokenDesc::~AETokenDesc()
{
	UpdateDesc();		
}



void AETokenDesc::SetPropertyCode(DescType propertyCode)
{
	mTokenRecord.propertyCode = propertyCode;
}


void AETokenDesc::SetDispatchClass(DescType dispatchClass)
{
	mTokenRecord.dispatchClass = dispatchClass;
}



void AETokenDesc::SetObjectClass(DescType objectClass)
{
	mTokenRecord.objectClass = objectClass;
}


void AETokenDesc::SetElementNumber(TAEListIndex number)
{
	mTokenRecord.elementNumber = number;
}


void AETokenDesc::SetWindow(WindowPtr wind)
{
	mTokenRecord.window = wind;
}


void AETokenDesc::UpdateDesc()
{
	OSErr	err = ::AEReplaceDescData(mTokenDesc->descriptorType, &mTokenRecord, sizeof(CoreTokenRecord), mTokenDesc);
	ThrowIfOSErr(err);
}
