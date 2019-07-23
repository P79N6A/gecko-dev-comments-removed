




































#include "nsPrivateTextRange.h"


nsPrivateTextRange::nsPrivateTextRange(PRUint16 aRangeStart, PRUint16 aRangeEnd, PRUint16 aRangeType)
:	mRangeStart(aRangeStart),
	mRangeEnd(aRangeEnd),
	mRangeType(aRangeType)
{
}

nsPrivateTextRange::~nsPrivateTextRange(void)
{
}

NS_IMPL_ISUPPORTS1(nsPrivateTextRange, nsIPrivateTextRange)

NS_METHOD nsPrivateTextRange::GetRangeStart(PRUint16* aRangeStart)
{
	*aRangeStart = mRangeStart;
	return NS_OK;
}

NS_METHOD nsPrivateTextRange::SetRangeStart(PRUint16 aRangeStart) 
{
	mRangeStart = aRangeStart;
	return NS_OK;
}

NS_METHOD nsPrivateTextRange::GetRangeEnd(PRUint16* aRangeEnd)
{
	*aRangeEnd = mRangeEnd;
	return NS_OK;
}

NS_METHOD nsPrivateTextRange::SetRangeEnd(PRUint16 aRangeEnd)
{
	mRangeEnd = aRangeEnd;
	return NS_OK;
}

NS_METHOD nsPrivateTextRange::GetRangeType(PRUint16* aRangeType)
{
	*aRangeType = mRangeType;
	return NS_OK;
}

NS_METHOD nsPrivateTextRange::SetRangeType(PRUint16 aRangeType)
{
	mRangeType = aRangeType;
	return NS_OK;
}

NS_IMPL_ISUPPORTS1(nsPrivateTextRangeList, nsIPrivateTextRangeList)

void nsPrivateTextRangeList::AppendTextRange(nsRefPtr<nsPrivateTextRange>& aRange)
{
	mList.AppendElement(aRange);
}

NS_METHOD nsPrivateTextRangeList::GetLength(PRUint16* aLength)
{
	*aLength = NS_STATIC_CAST(PRUint16, mList.Length());
	return NS_OK;
}

NS_METHOD nsPrivateTextRangeList::Item(PRUint16 aIndex, nsIPrivateTextRange** aReturn)
{
	*aReturn = mList.ElementAt(aIndex);
	if (*aReturn) {
		NS_ADDREF(*aReturn);
		return NS_OK;
	}
	return NS_ERROR_FAILURE;
}

