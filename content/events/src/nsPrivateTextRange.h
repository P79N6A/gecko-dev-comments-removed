




































#ifndef nsPrivateTextRange_h__
#define nsPrivateTextRange_h__

#include "nsIPrivateTextRange.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"

class nsPrivateTextRange : public nsIPrivateTextRange 
{
	NS_DECL_ISUPPORTS
public:

	nsPrivateTextRange(PRUint16 aRangeStart, PRUint16 aRangeEnd, PRUint16 aRangeType);
	virtual ~nsPrivateTextRange(void);

	NS_IMETHOD    GetRangeStart(PRUint16* aRangeStart);
	NS_IMETHOD    SetRangeStart(PRUint16 aRangeStart);

	NS_IMETHOD    GetRangeEnd(PRUint16* aRangeEnd);
	NS_IMETHOD    SetRangeEnd(PRUint16 aRangeEnd);

	NS_IMETHOD    GetRangeType(PRUint16* aRangeType);
	NS_IMETHOD    SetRangeType(PRUint16 aRangeType);

protected:

	PRUint16	mRangeStart;
	PRUint16	mRangeEnd;
	PRUint16	mRangeType;
};

class nsPrivateTextRangeList: public nsIPrivateTextRangeList 
{
	NS_DECL_ISUPPORTS
public:
	nsPrivateTextRangeList(PRUint16 aLength) : mList(aLength) {}

	void          AppendTextRange(nsRefPtr<nsPrivateTextRange>& aRange);

	NS_IMETHOD    GetLength(PRUint16* aLength);

	NS_IMETHOD    Item(PRUint16 aIndex, nsIPrivateTextRange** aReturn);
protected:
	nsTArray<nsRefPtr<nsPrivateTextRange> > mList;
};


#endif
