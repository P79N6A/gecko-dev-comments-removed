









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

static const uint32_t kStyleChangeBufferSize = 10;



class nsStyleChangeList {
public:
  nsStyleChangeList();
  ~nsStyleChangeList();

  int32_t Count(void) const {
    return mCount;
  }

  


  nsresult ChangeAt(int32_t aIndex, nsIFrame*& aFrame, nsIContent*& aContent,
                    nsChangeHint& aHint) const;

  



  nsresult ChangeAt(int32_t aIndex, const nsStyleChangeData** aChangeData) const;

  nsresult AppendChange(nsIFrame* aFrame, nsIContent* aContent, nsChangeHint aHint);

  void Clear(void);

protected:
  nsStyleChangeList&  operator=(const nsStyleChangeList& aCopy);
  bool                operator==(const nsStyleChangeList& aOther) const;

  nsStyleChangeData*  mArray;
  int32_t             mArraySize;
  int32_t             mCount;
  nsStyleChangeData   mBuffer[kStyleChangeBufferSize];

private:
  nsStyleChangeList(const nsStyleChangeList&) MOZ_DELETE;
};


#endif 
