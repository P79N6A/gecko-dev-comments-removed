




#ifndef nsPrivateTextRange_h__
#define nsPrivateTextRange_h__

#include "nsIPrivateTextRange.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "mozilla/Attributes.h"

class nsPrivateTextRange MOZ_FINAL : public nsIPrivateTextRange
{
	NS_DECL_ISUPPORTS
public:

	nsPrivateTextRange(const nsTextRange &aTextRange);
	virtual ~nsPrivateTextRange(void);

	NS_IMETHOD    GetRangeStart(uint16_t* aRangeStart) MOZ_OVERRIDE;
	NS_IMETHOD    GetRangeEnd(uint16_t* aRangeEnd) MOZ_OVERRIDE;
	NS_IMETHOD    GetRangeType(uint16_t* aRangeType) MOZ_OVERRIDE;
	NS_IMETHOD    GetRangeStyle(nsTextRangeStyle* aRangeStyle) MOZ_OVERRIDE;

protected:

	uint16_t	mRangeStart;
	uint16_t	mRangeEnd;
	uint16_t	mRangeType;
	nsTextRangeStyle mRangeStyle;
};

class nsPrivateTextRangeList MOZ_FINAL : public nsIPrivateTextRangeList
{
	NS_DECL_ISUPPORTS
public:
	nsPrivateTextRangeList(uint16_t aLength) : mList(aLength) {}

	void          AppendTextRange(nsRefPtr<nsPrivateTextRange>& aRange);

	NS_IMETHOD_(uint16_t)    GetLength() MOZ_OVERRIDE;

	NS_IMETHOD_(already_AddRefed<nsIPrivateTextRange>)    Item(uint16_t aIndex) MOZ_OVERRIDE;
protected:
	nsTArray<nsRefPtr<nsPrivateTextRange> > mList;
};


#endif
