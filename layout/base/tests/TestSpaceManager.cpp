



































#include <stdio.h>
#include "nscore.h"
#include "nsCRT.h"
#include "nsSpaceManager.h"

class MySpaceManager: public nsSpaceManager {
public:
  MySpaceManager(nsIPresShell* aPresShell, nsIFrame* aFrame)
      : nsSpaceManager(aPresShell, aFrame) {}

  PRBool  TestAddBand();
  PRBool  TestAddBandOverlap();
  PRBool  TestAddRectToBand();
  PRBool  TestRemoveRegion();
  PRBool  TestGetBandData();

  struct BandInfo {
    nscoord   yOffset;
    nscoord   height;
    BandRect* firstRect;
    PRIntn    numRects;
  };

  struct BandsInfo {
    PRIntn   numBands;
    BandInfo bands[25];
  };

protected:
  void  GetBandsInfo(BandsInfo&);
};

void MySpaceManager::GetBandsInfo(BandsInfo& aBandsInfo)
{
  aBandsInfo.numBands = 0;

  if (!mBandList.IsEmpty()) {
    BandRect* band = mBandList.Head();
    while (nsnull != band) {
      BandInfo& info = aBandsInfo.bands[aBandsInfo.numBands];

      info.yOffset = band->mTop;
      info.height = band->mBottom - band->mTop;
      info.firstRect = band;

      aBandsInfo.numBands++;

      
      info.numRects = 0;
      while (info.yOffset == band->mTop) {
        info.numRects++;

        band = band->Next();
        if (band == &mBandList) {
          
          band = nsnull;
          break;
        }
      }
    }
  }
}












PRBool MySpaceManager::TestAddBand()
{
  BandsInfo bandsInfo;
  nsresult  status;
  
  
  ClearRegions();
  NS_ASSERTION(mBandList.IsEmpty(), "clear regions failed");

  
  
  
  status = AddRectRegion((nsIFrame*)0x01, nsRect(10, 100, 100, 100));
  if (NS_FAILED(status)) {
    printf("TestAddBand: add failed (#1)\n");
    return PR_FALSE;
  }
  GetBandsInfo(bandsInfo);
  if (bandsInfo.numBands != 1) {
    printf("TestAddBand: wrong number of bands (#1): %i\n", bandsInfo.numBands);
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].yOffset != 100) || (bandsInfo.bands[0].height != 100)) {
    printf("TestAddBand: wrong band size (#1)\n");
    return PR_FALSE;
  }

  
  
  status = AddRectRegion((nsIFrame*)0x02, nsRect(10, -10, 100, 20));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");
  GetBandsInfo(bandsInfo);
  if (bandsInfo.numBands != 2) {
    printf("TestAddBand: wrong number of bands (#2): %i\n", bandsInfo.numBands);
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].yOffset != -10) || (bandsInfo.bands[0].height != 20) ||
      (bandsInfo.bands[1].yOffset != 100) || (bandsInfo.bands[1].height != 100)) {
    printf("TestAddBand: wrong band sizes (#2)\n");
    return PR_FALSE;
  }

  
  
  status = AddRectRegion((nsIFrame*)0x03, nsRect(10, 40, 100, 30));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");
  GetBandsInfo(bandsInfo);
  if (bandsInfo.numBands != 3) {
    printf("TestAddBand: wrong number of bands (#3): %i\n", bandsInfo.numBands);
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].yOffset != -10) || (bandsInfo.bands[0].height != 20) ||
      (bandsInfo.bands[1].yOffset != 40) || (bandsInfo.bands[1].height != 30) ||
      (bandsInfo.bands[2].yOffset != 100) || (bandsInfo.bands[2].height != 100)) {
    printf("TestAddBand: wrong band sizes (#3)\n");
    return PR_FALSE;
  }

  
  
  status = AddRectRegion((nsIFrame*)0x04, nsRect(10, 210, 100, 100));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");
  GetBandsInfo(bandsInfo);
  if (bandsInfo.numBands != 4) {
    printf("TestAddBand: wrong number of bands (#4): %i\n", bandsInfo.numBands);
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].yOffset != -10) || (bandsInfo.bands[0].height != 20) ||
      (bandsInfo.bands[1].yOffset != 40) || (bandsInfo.bands[1].height != 30) ||
      (bandsInfo.bands[2].yOffset != 100) || (bandsInfo.bands[2].height != 100) ||
      (bandsInfo.bands[3].yOffset != 210) || (bandsInfo.bands[3].height != 100)) {
    printf("TestAddBand: wrong band sizes (#4)\n");
    return PR_FALSE;
  }

  return PR_TRUE;
}








PRBool MySpaceManager::TestAddBandOverlap()
{
  BandsInfo bandsInfo;
  nsresult  status;
  
  
  ClearRegions();
  NS_ASSERTION(mBandList.IsEmpty(), "clear regions failed");

  
  status = AddRectRegion((nsIFrame*)0x01, nsRect(100, 25, 100, 100));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");

  
  
  status = AddRectRegion((nsIFrame*)0x02, nsRect(10, -25, 50, 100));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");
  GetBandsInfo(bandsInfo);
  if (bandsInfo.numBands != 3) {
    printf("TestAddBandOverlap: wrong number of bands (#1): %i\n", bandsInfo.numBands);
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].yOffset != -25) || (bandsInfo.bands[0].height != 50) ||
      (bandsInfo.bands[1].yOffset != 25) || (bandsInfo.bands[1].height != 50) ||
      (bandsInfo.bands[2].yOffset != 75) || (bandsInfo.bands[2].height != 50)) {
    printf("TestAddBandOverlap: wrong band sizes (#1)\n");
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].numRects != 1) ||
      (bandsInfo.bands[1].numRects != 2) ||
      (bandsInfo.bands[2].numRects != 1)) {
    printf("TestAddBandOverlap: wrong number of rects (#1)\n");
    return PR_FALSE;
  }

  
  
  status = AddRectRegion((nsIFrame*)0x03, nsRect(200, -15, 50, 10));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");
  GetBandsInfo(bandsInfo);
  if (bandsInfo.numBands != 5) {
    printf("TestAddBandOverlap: wrong number of bands (#2): %i\n", bandsInfo.numBands);
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].yOffset != -25) || (bandsInfo.bands[0].height != 10) ||
      (bandsInfo.bands[1].yOffset != -15) || (bandsInfo.bands[1].height != 10) ||
      (bandsInfo.bands[2].yOffset != -5) || (bandsInfo.bands[2].height != 30) ||
      (bandsInfo.bands[3].yOffset != 25) || (bandsInfo.bands[3].height != 50) ||
      (bandsInfo.bands[4].yOffset != 75) || (bandsInfo.bands[4].height != 50)) {
    printf("TestAddBandOverlap: wrong band sizes (#2)\n");
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].numRects != 1) ||
      (bandsInfo.bands[1].numRects != 2) ||
      (bandsInfo.bands[2].numRects != 1) ||
      (bandsInfo.bands[3].numRects != 2) ||
      (bandsInfo.bands[4].numRects != 1)) {
    printf("TestAddBandOverlap: wrong number of rects (#2)\n");
    return PR_FALSE;
  }

  
  
  status = AddRectRegion((nsIFrame*)0x04, nsRect(200, 100, 50, 50));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");
  GetBandsInfo(bandsInfo);
  if (bandsInfo.numBands != 7) {
    printf("TestAddBandOverlap: wrong number of bands (#3): %i\n", bandsInfo.numBands);
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].yOffset != -25) || (bandsInfo.bands[0].height != 10) ||
      (bandsInfo.bands[1].yOffset != -15) || (bandsInfo.bands[1].height != 10) ||
      (bandsInfo.bands[2].yOffset != -5) || (bandsInfo.bands[2].height != 30) ||
      (bandsInfo.bands[3].yOffset != 25) || (bandsInfo.bands[3].height != 50) ||
      (bandsInfo.bands[4].yOffset != 75) || (bandsInfo.bands[4].height != 25) ||
      (bandsInfo.bands[5].yOffset != 100) || (bandsInfo.bands[5].height != 25) ||
      (bandsInfo.bands[6].yOffset != 125) || (bandsInfo.bands[6].height != 25)) {
    printf("TestAddBandOverlap: wrong band sizes (#3)\n");
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].numRects != 1) ||
      (bandsInfo.bands[1].numRects != 2) ||
      (bandsInfo.bands[2].numRects != 1) ||
      (bandsInfo.bands[3].numRects != 2) ||
      (bandsInfo.bands[4].numRects != 1) ||
      (bandsInfo.bands[5].numRects != 2) ||
      (bandsInfo.bands[6].numRects != 1)) {
    printf("TestAddBandOverlap: wrong number of rects (#3)\n");
    return PR_FALSE;
  }

  
  
  ClearRegions();
  status = AddRectRegion((nsIFrame*)0x01, nsRect(100, 100, 100, 100));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");

  
  status = AddRectRegion((nsIFrame*)0x02, nsRect(200, 50, 100, 200));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");

  GetBandsInfo(bandsInfo);
  if (bandsInfo.numBands != 3) {
    printf("TestAddBandOverlap: wrong number of bands (#4): %i\n", bandsInfo.numBands);
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].yOffset != 50) || (bandsInfo.bands[0].height != 50) ||
      (bandsInfo.bands[1].yOffset != 100) || (bandsInfo.bands[1].height != 100) ||
      (bandsInfo.bands[2].yOffset != 200) || (bandsInfo.bands[2].height != 50)) {
    printf("TestAddBandOverlap: wrong band sizes (#4)\n");
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].numRects != 1) ||
      (bandsInfo.bands[1].numRects != 2) ||
      (bandsInfo.bands[2].numRects != 1)) {
    printf("TestAddBandOverlap: wrong number of rects (#4)\n");
    return PR_FALSE;
  }

  return PR_TRUE;
}










PRBool MySpaceManager::TestAddRectToBand()
{
  BandsInfo bandsInfo;
  BandRect* bandRect;
  nsresult  status;

  
  ClearRegions();
  NS_ASSERTION(mBandList.IsEmpty(), "clear regions failed");

  
  status = AddRectRegion((nsIFrame*)0x01, nsRect(100, 100, 100, 100));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");

  
  
  status = AddRectRegion((nsIFrame*)0x02, nsRect(10, 100, 50, 100));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");
  GetBandsInfo(bandsInfo);
  if (bandsInfo.numBands != 1) {
    printf("TestAddRectToBand: wrong number of bands (#1): %i\n", bandsInfo.numBands);
    return PR_FALSE;
  }
  if (bandsInfo.bands[0].numRects != 2) {
    printf("TestAddRectToBand: wrong number of rects (#1): %i\n", bandsInfo.bands[0].numRects);
    return PR_FALSE;
  }
  bandRect = bandsInfo.bands[0].firstRect;
  if ((bandRect->mLeft != 10) || (bandRect->mRight != 60)) {
    printf("TestAddRectToBand: wrong first rect (#1)\n");
    return PR_FALSE;
  }
  bandRect = bandRect->Next();
  if ((bandRect->mLeft != 100) || (bandRect->mRight != 200)) {
    printf("TestAddRectToBand: wrong second rect (#1)\n");
    return PR_FALSE;
  }

  
  
  status = AddRectRegion((nsIFrame*)0x03, nsRect(250, 100, 100, 100));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");
  GetBandsInfo(bandsInfo);
  NS_ASSERTION(bandsInfo.numBands == 1, "wrong number of bands");
  if (bandsInfo.bands[0].numRects != 3) {
    printf("TestAddRectToBand: wrong number of rects (#2): %i\n", bandsInfo.bands[0].numRects);
    return PR_FALSE;
  }
  bandRect = bandsInfo.bands[0].firstRect;
  if ((bandRect->mLeft != 10) || (bandRect->mRight != 60)) {
    printf("TestAddRectToBand: wrong first rect (#2)\n");
    return PR_FALSE;
  }
  bandRect = bandRect->Next();
  if ((bandRect->mLeft != 100) || (bandRect->mRight != 200)) {
    printf("TestAddRectToBand: wrong second rect (#2)\n");
    return PR_FALSE;
  }
  bandRect = bandRect->Next();
  if ((bandRect->mLeft != 250) || (bandRect->mRight != 350)) {
    printf("TestAddRectToBand: wrong third rect (#2)\n");
    return PR_FALSE;
  }

  
  
  
  status = AddRectRegion((nsIFrame*)0x04, nsRect(80, 100, 40, 100));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");
  GetBandsInfo(bandsInfo);
  NS_ASSERTION(bandsInfo.numBands == 1, "wrong number of bands");
  if (bandsInfo.bands[0].numRects != 5) {
    printf("TestAddRectToBand: wrong number of rects (#3): %i\n", bandsInfo.bands[0].numRects);
    return PR_FALSE;
  }
  bandRect = bandsInfo.bands[0].firstRect;
  if ((bandRect->mLeft != 10) || (bandRect->mRight != 60)) {
    printf("TestAddRectToBand: wrong first rect (#3)\n");
    return PR_FALSE;
  }
  bandRect = bandRect->Next();
  NS_ASSERTION(1 == bandRect->mNumFrames, "unexpected shared rect");
  if ((bandRect->mLeft != 80) || (bandRect->mRight != 100)) {
    printf("TestAddRectToBand: wrong second rect (#3)\n");
    return PR_FALSE;
  }
  bandRect = bandRect->Next();
  if ((bandRect->mLeft != 100) || (bandRect->mRight != 120) ||
      (bandRect->mNumFrames != 2) || !bandRect->IsOccupiedBy((nsIFrame*)0x04)) {
    printf("TestAddRectToBand: wrong third rect (#3)\n");
    return PR_FALSE;
  }
  bandRect = bandRect->Next();
  NS_ASSERTION(1 == bandRect->mNumFrames, "unexpected shared rect");
  if ((bandRect->mLeft != 120) || (bandRect->mRight != 200)) {
    printf("TestAddRectToBand: wrong fourth rect (#3)\n");
    return PR_FALSE;
  }
  bandRect = bandRect->Next();
  if ((bandRect->mLeft != 250) || (bandRect->mRight != 350)) {
    printf("TestAddRectToBand: wrong fifth rect (#3)\n");
    return PR_FALSE;
  }

  
  
  
  status = AddRectRegion((nsIFrame*)0x05, nsRect(50, 100, 20, 100));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");
  GetBandsInfo(bandsInfo);
  NS_ASSERTION(bandsInfo.numBands == 1, "wrong number of bands");
  if (bandsInfo.bands[0].numRects != 7) {
    printf("TestAddRectToBand: wrong number of rects (#4): %i\n", bandsInfo.bands[0].numRects);
    return PR_FALSE;
  }
  bandRect = bandsInfo.bands[0].firstRect;
  NS_ASSERTION(1 == bandRect->mNumFrames, "unexpected shared rect");
  if ((bandRect->mLeft != 10) || (bandRect->mRight != 50)) {
    printf("TestAddRectToBand: wrong first rect (#4)\n");
    return PR_FALSE;
  }
  bandRect = bandRect->Next();
  if ((bandRect->mLeft != 50) || (bandRect->mRight != 60) ||
      (bandRect->mNumFrames != 2) || !bandRect->IsOccupiedBy((nsIFrame*)0x05)) {
    printf("TestAddRectToBand: wrong second rect (#4)\n");
    return PR_FALSE;
  }
  bandRect = bandRect->Next();
  NS_ASSERTION(1 == bandRect->mNumFrames, "unexpected shared rect");
  if ((bandRect->mLeft != 60) || (bandRect->mRight != 70)) {
    printf("TestAddRectToBand: wrong third rect (#4)\n");
    return PR_FALSE;
  }
  bandRect = bandRect->Next();
  NS_ASSERTION(1 == bandRect->mNumFrames, "unexpected shared rect");
  if ((bandRect->mLeft != 80) || (bandRect->mRight != 100)) {
    printf("TestAddRectToBand: wrong fourth rect (#4)\n");
    return PR_FALSE;
  }

  
  
  
  status = AddRectRegion((nsIFrame*)0x06, nsRect(20, 100, 20, 100));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");
  GetBandsInfo(bandsInfo);
  NS_ASSERTION(bandsInfo.numBands == 1, "wrong number of bands");
  if (bandsInfo.bands[0].numRects != 9) {
    printf("TestAddRectToBand: wrong number of rects (#5): %i\n", bandsInfo.bands[0].numRects);
    return PR_FALSE;
  }
  bandRect = bandsInfo.bands[0].firstRect;
  NS_ASSERTION(1 == bandRect->mNumFrames, "unexpected shared rect");
  if ((bandRect->mLeft != 10) || (bandRect->mRight != 20)) {
    printf("TestAddRectToBand: wrong first rect (#5)\n");
    return PR_FALSE;
  }
  bandRect = bandRect->Next();
  if ((bandRect->mLeft != 20) || (bandRect->mRight != 40) ||
      (bandRect->mNumFrames != 2) || !bandRect->IsOccupiedBy((nsIFrame*)0x06)) {
    printf("TestAddRectToBand: wrong second rect (#5)\n");
    return PR_FALSE;
  }
  bandRect = bandRect->Next();
  NS_ASSERTION(1 == bandRect->mNumFrames, "unexpected shared rect");
  if ((bandRect->mLeft != 40) || (bandRect->mRight != 50)) {
    printf("TestAddRectToBand: wrong third rect (#5)\n");
    return PR_FALSE;
  }
  bandRect = bandRect->Next();
  if ((bandRect->mLeft != 50) || (bandRect->mRight != 60) || (bandRect->mNumFrames != 2)) {
    printf("TestAddRectToBand: wrong fourth rect (#5)\n");
    return PR_FALSE;
  }

  
  
  status = AddRectRegion((nsIFrame*)0x07, nsRect(0, 100, 30, 100));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");
  GetBandsInfo(bandsInfo);
  NS_ASSERTION(bandsInfo.numBands == 1, "wrong number of bands");
  if (bandsInfo.bands[0].numRects != 11) {
    printf("TestAddRectToBand: wrong number of rects (#6): %i\n", bandsInfo.bands[0].numRects);
    return PR_FALSE;
  }
  bandRect = bandsInfo.bands[0].firstRect;
  NS_ASSERTION(1 == bandRect->mNumFrames, "unexpected shared rect");
  if ((bandRect->mLeft != 0) || (bandRect->mRight != 10)) {
    printf("TestAddRectToBand: wrong first rect (#6)\n");
    return PR_FALSE;
  }
  bandRect = bandRect->Next();
  if ((bandRect->mLeft != 10) || (bandRect->mRight != 20) ||
      (bandRect->mNumFrames != 2) || !bandRect->IsOccupiedBy((nsIFrame*)0x07)) {
    printf("TestAddRectToBand: wrong second rect (#6)\n");
    return PR_FALSE;
  }
  bandRect = bandRect->Next();
  if ((bandRect->mLeft != 20) || (bandRect->mRight != 30) ||
      (bandRect->mNumFrames != 3) || !bandRect->IsOccupiedBy((nsIFrame*)0x07)) {
    printf("TestAddRectToBand: wrong third rect (#6)\n");
    return PR_FALSE;
  }
  bandRect = bandRect->Next();
  if ((bandRect->mLeft != 30) || (bandRect->mRight != 40) || (bandRect->mNumFrames != 2)) {
    printf("TestAddRectToBand: wrong fourth rect (#6)\n");
    return PR_FALSE;
  }
  bandRect = bandRect->Next();
  NS_ASSERTION(1 == bandRect->mNumFrames, "unexpected shared rect");
  if ((bandRect->mLeft != 40) || (bandRect->mRight != 50)) {
    printf("TestAddRectToBand: wrong fifth rect (#6)\n");
    return PR_FALSE;
  }

  return PR_TRUE;
}








PRBool MySpaceManager::TestRemoveRegion()
{
  BandsInfo bandsInfo;
  BandRect* bandRect;
  nsresult  status;

  
  ClearRegions();
  NS_ASSERTION(mBandList.IsEmpty(), "clear regions failed");

  
  
  status = AddRectRegion((nsIFrame*)0x01, nsRect(10, 100, 100, 100));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");
  status = RemoveRegion((nsIFrame*)0x01);
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");
  GetBandsInfo(bandsInfo);
  if (bandsInfo.numBands != 0) {
    printf("TestRemoveRegion: wrong number of bands (#1): %i\n", bandsInfo.numBands);
    return PR_FALSE;
  }

  
  
  
  status = AddRectRegion((nsIFrame*)0x01, nsRect(10, 100, 100, 100));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");
  status = AddRectRegion((nsIFrame*)0x02, nsRect(40, 100, 20, 100));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");

  
  GetBandsInfo(bandsInfo);
  if (bandsInfo.bands[0].numRects != 3) {
    printf("TestRemoveRegion: wrong number of rects (#2): %i\n", bandsInfo.bands[0].numRects);
    return PR_FALSE;
  }

  
  status = RemoveRegion((nsIFrame*)0x02);
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");
  GetBandsInfo(bandsInfo);
  if (bandsInfo.bands[0].numRects != 1) {
    printf("TestRemoveRegion: failed to coalesce adjacent rects (#2)\n");
    return PR_FALSE;
  }
  bandRect = bandsInfo.bands[0].firstRect;
  if ((bandRect->mLeft != 10) || (bandRect->mRight != 110)) {
    printf("TestRemoveRegion: wrong size rect (#2)\n");
    return PR_FALSE;
  }

  
  
  status = AddRectRegion((nsIFrame*)0x02, nsRect(10, 140, 20, 20));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");

  
  GetBandsInfo(bandsInfo);
  if (bandsInfo.numBands != 3) {
    printf("TestRemoveRegion: wrong number of bands (#3): %i\n", bandsInfo.numBands);
    return PR_FALSE;
  }
  if (bandsInfo.bands[0].numRects != 1) {
    printf("TestRemoveRegion: band #1 wrong number of rects (#3): %i\n", bandsInfo.bands[0].numRects);
    return PR_FALSE;
  }
  if (bandsInfo.bands[1].numRects != 2) {
    printf("TestRemoveRegion: band #2 wrong number of rects (#3): %i\n", bandsInfo.bands[1].numRects);
    return PR_FALSE;
  }
  if (bandsInfo.bands[2].numRects != 1) {
    printf("TestRemoveRegion: band #3 wrong number of rects (#3): %i\n", bandsInfo.bands[2].numRects);
    return PR_FALSE;
  }

  
  status = RemoveRegion((nsIFrame*)0x02);
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");
  GetBandsInfo(bandsInfo);
  if (bandsInfo.bands[0].numRects != 1) {
    printf("TestRemoveRegion: failed to coalesce adjacent rects (#3)\n");
    return PR_FALSE;
  }
  bandRect = bandsInfo.bands[0].firstRect;
  if ((bandRect->mLeft != 10) || (bandRect->mRight != 110)) {
    printf("TestRemoveRegion: wrong size rect (#3)\n");
    return PR_FALSE;
  }

  return PR_TRUE;
}


PRBool MySpaceManager::TestGetBandData()
{
  nsresult  status;
  nscoord   yMost;

  
  ClearRegions();
  NS_ASSERTION(mBandList.IsEmpty(), "clear regions failed");

  
  if (NS_ERROR_ABORT != YMost(yMost)) {
    printf("TestGetBandData: YMost() returned wrong result (#1)\n");
    return PR_FALSE;
  }

  
  status = AddRectRegion((nsIFrame*)0x01, nsRect(100, 100, 100, 100));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");

  status = AddRectRegion((nsIFrame*)0x02, nsRect(300, 100, 100, 100));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");

  status = AddRectRegion((nsIFrame*)0x03, nsRect(500, 100, 100, 100));
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");

  
  if ((NS_OK != YMost(yMost)) || (yMost != 200)) {
    printf("TestGetBandData: YMost() returned wrong value (#2)\n");
    return PR_FALSE;
  }

  
  
  nsBandData      bandData;
  nsBandTrapezoid trapezoids[16];
  bandData.mSize = 16;
  bandData.mTrapezoids = trapezoids;
  status = GetBandData(100, nsSize(10000,10000), bandData);
  NS_ASSERTION(NS_SUCCEEDED(status), "unexpected status");

  
  if (bandData.mCount != 7) {
    printf("TestGetBandData: wrong trapezoid count (#3)\n");
    return PR_FALSE;
  }
  
  
  
  bandData.mSize = 3;
  status = GetBandData(100, nsSize(10000,10000), bandData);
  if (NS_SUCCEEDED(status)) {
    printf("TestGetBandData: ignored band data count (#4)\n");
    return PR_FALSE;
  }

  
  
  if (bandData.mCount <= bandData.mSize) {
    printf("TestGetBandData: bad band data count (#5)\n");
    return PR_FALSE;
  }

  

  return PR_TRUE;
}




int main(int argc, char** argv)
{
  
  MySpaceManager* spaceMgr = new MySpaceManager(nsnull, nsnull);
  
  
  if (!spaceMgr->TestAddBand()) {
    delete spaceMgr;
    return -1;
  }

  
  if (!spaceMgr->TestAddBandOverlap()) {
    delete spaceMgr;
    return -1;
  }

  
  if (!spaceMgr->TestAddRectToBand()) {
    delete spaceMgr;
    return -1;
  }

  
  if (!spaceMgr->TestRemoveRegion()) {
    return -1;
  }

  
  if (!spaceMgr->TestGetBandData()) {
    return -1;
  }

  delete spaceMgr;
  return 0;
}
