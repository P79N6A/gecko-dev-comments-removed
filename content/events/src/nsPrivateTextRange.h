




































#ifndef nsPrivateTextRange_h__
#define nsPrivateTextRange_h__

#include "nsIPrivateTextRange.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"

class nsPrivateTextRange : public nsIPrivateTextRange 
{
	NS_DECL_ISUPPORTS
public:

	nsPrivateTextRange(const nsTextRange &aTextRange);
	virtual ~nsPrivateTextRange(void);

	NS_IMETHOD    GetRangeStart(PRUint16* aRangeStart);
	NS_IMETHOD    SetRangeStart(PRUint16 aRangeStart);

	NS_IMETHOD    GetRangeEnd(PRUint16* aRangeEnd);
	NS_IMETHOD    SetRangeEnd(PRUint16 aRangeEnd);

	NS_IMETHOD    GetRangeType(PRUint16* aRangeType);
	NS_IMETHOD    SetRangeType(PRUint16 aRangeType);

	NS_IMETHOD    GetRangeStyle(nsTextRangeStyle* aRangeStyle);

protected:

	PRUint16	mRangeStart;
	PRUint16	mRangeEnd;
	PRUint16	mRangeType;
	nsTextRangeStyle mRangeStyle;
};

class nsPrivateTextRangeList: public nsIPrivateTextRangeList 
{
	NS_DECL_ISUPPORTS
public:
	nsPrivateTextRangeList(PRUint16 aLength) : mList(aLength) {}

	void          AppendTextRange(nsRefPtr<nsPrivateTextRange>& aRange);

	NS_IMETHOD_(PRUint16)    GetLength();

	NS_IMETHOD_(already_AddRefed<nsIPrivateTextRange>)    Item(PRUint16 aIndex);
protected:
	nsTArray<nsRefPtr<nsPrivateTextRange> > mList;
};


#endif
