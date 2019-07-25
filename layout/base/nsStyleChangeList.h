









































#ifndef nsStyleChangeList_h___
#define nsStyleChangeList_h___

#include "mozilla/Attributes.h"

#include "nsError.h"
#include "nsChangeHint.h"

class nsIFrame;
class nsIContent;


struct nsStyleChangeData {
  nsIFrame*   mFrame;
  nsIContent* mContent;
  nsChangeHint mHint;
};

static const PRUint32 kStyleChangeBufferSize = 10;



class nsStyleChangeList {
public:
  nsStyleChangeList();
  ~nsStyleChangeList();

  PRInt32 Count(void) const {
    return mCount;
  }

  


  nsresult ChangeAt(PRInt32 aIndex, nsIFrame*& aFrame, nsIContent*& aContent,
                    nsChangeHint& aHint) const;

  



  nsresult ChangeAt(PRInt32 aIndex, const nsStyleChangeData** aChangeData) const;

  nsresult AppendChange(nsIFrame* aFrame, nsIContent* aContent, nsChangeHint aHint);

  void Clear(void);

protected:
  nsStyleChangeList&  operator=(const nsStyleChangeList& aCopy);
  bool                operator==(const nsStyleChangeList& aOther) const;

  nsStyleChangeData*  mArray;
  PRInt32             mArraySize;
  PRInt32             mCount;
  nsStyleChangeData   mBuffer[kStyleChangeBufferSize];

private:
  nsStyleChangeList(const nsStyleChangeList&) MOZ_DELETE;
};


#endif 
