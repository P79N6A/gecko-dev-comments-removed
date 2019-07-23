






































#ifndef nsBlockBandData_h___
#define nsBlockBandData_h___

#include "nsSpaceManager.h"

class nsPresContext;


#define NS_BLOCK_BAND_DATA_TRAPS 6





class nsBlockBandData : public nsBandData {
public:
  nsBlockBandData();
  ~nsBlockBandData();

  
  nsresult Init(nsSpaceManager* aSpaceManager, const nsSize& aSpace);

  
  
  nsresult GetAvailableSpace(nscoord aY, PRBool aRelaxHeightConstraint,
                             nsRect& aResult);

  
  PRInt32 GetTrapezoidCount() const {
    return mCount;
  }

  const nsBandTrapezoid* GetTrapezoid(PRInt32 aIndex) const {
    return &mTrapezoids[aIndex];
  }

  
  
  
  
  PRInt32 GetFloatCount() const {
    return mLeftFloats + mRightFloats;
  }
  PRInt32 GetLeftFloatCount() const {
    return mLeftFloats;
  }
  PRInt32 GetRightFloatCount() const {
    return mRightFloats;
  }

#ifdef DEBUG
  void List();
#endif

protected:

  





  nsresult GetBandData(nscoord aY, PRBool aRelaxHeightConstraint);

  
  nsSpaceManager* mSpaceManager;
  nscoord mSpaceManagerX, mSpaceManagerY;

  
  nsSize mSpace;

  
  nsBandTrapezoid mData[NS_BLOCK_BAND_DATA_TRAPS];

  
  nsRect mAvailSpace;

  
  
  
  
  PRInt32 mLeftFloats, mRightFloats;

  void ComputeAvailSpaceRect();
};

#endif 
