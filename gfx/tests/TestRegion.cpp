




































#include "TestHarness.h"
#include "nsRegion.h"

class TestLargestRegion {
  static PRBool TestSingleRect(nsRect r) {
    nsRegion region(r);
    if (!region.GetLargestRectangle().IsEqualInterior(r)) {
      fail("largest rect of singleton %d %d %d %d", r.x, r.y, r.width, r.height);
      return PR_FALSE;
    }
    return PR_TRUE;
  }
  
  static PRBool TestNonRectangular() {
    nsRegion r(nsRect(0, 0, 30, 30));

    const int nTests = 19;
    struct {
      nsRect rect;
      PRInt64 expectedArea;
    } tests[nTests] = {
      
      { nsRect(0, 0, 20, 10), 600 },
      { nsRect(10, 0, 20, 10), 600 },
      { nsRect(10, 20, 20, 10), 600 },
      { nsRect(0, 20, 20, 10), 600 },
      
      { nsRect(0, 0, 10, 20), 600 },
      { nsRect(20, 0, 10, 20), 600 },
      { nsRect(20, 10, 10, 20), 600 },
      { nsRect(0, 10, 10, 20), 600 },
      
      { nsRect(10, 10, 10, 10), 300 },
      
      { nsRect(10, 0, 10, 30), 300 },
      
      { nsRect(0, 10, 30, 10), 300 },
      
      { nsRect(0, 0, 10, 10), 600 },
      { nsRect(20, 20, 10, 10), 600 },
      { nsRect(20, 0, 10, 10), 600 },
      { nsRect(0, 20, 10, 10), 600 },
      
      { nsRect(0, 0, 20, 20), 300 },
      { nsRect(10, 10, 20, 20), 300 },
      { nsRect(10, 0, 20, 20), 300 },
      { nsRect(0, 10, 20, 20), 300 }
    };

    PRBool success = PR_TRUE;
    for (PRInt32 i = 0; i < nTests; i++) {
      nsRegion r2;
      r2.Sub(r, tests[i].rect);

      if (!r2.IsComplex())
        fail("nsRegion code got unexpectedly smarter!");

      nsRect largest = r2.GetLargestRectangle();
      if (largest.width * largest.height != tests[i].expectedArea) {
        fail("Did not succesfully find largest rectangle in non-rectangular region on iteration %d", i);
        success = PR_FALSE;
      }
    }

    return success;
  }
  static PRBool TwoRectTest() {
    nsRegion r(nsRect(0, 0, 100, 100));
    const int nTests = 4;
    struct {
      nsRect rect1, rect2;
      PRInt64 expectedArea;
    } tests[nTests] = {
      { nsRect(0, 0, 75, 40),  nsRect(0, 60, 75, 40),  2500 },
      { nsRect(25, 0, 75, 40), nsRect(25, 60, 75, 40), 2500 },
      { nsRect(25, 0, 75, 40), nsRect(0, 60, 75, 40),  2000 },
      { nsRect(0, 0, 75, 40),  nsRect(25, 60, 75, 40), 2000 },
    };
    PRBool success = PR_TRUE;
    for (PRInt32 i = 0; i < nTests; i++) {
      nsRegion r2;

      r2.Sub(r, tests[i].rect1);
      r2.Sub(r2, tests[i].rect2);

      if (!r2.IsComplex())
        fail("nsRegion code got unexpectedly smarter!");

      nsRect largest = r2.GetLargestRectangle();
      if (largest.width * largest.height != tests[i].expectedArea) {
        fail("Did not succesfully find largest rectangle in two-rect-subtract region on iteration %d", i);
        success = PR_FALSE;
      }
    }
    return success;
  }
  static PRBool TestContainsSpecifiedRect() {
    nsRegion r(nsRect(0, 0, 100, 100));
    r.Or(r, nsRect(0, 300, 50, 50));
    if (!r.GetLargestRectangle(nsRect(0, 300, 10, 10)).IsEqualInterior(nsRect(0, 300, 50, 50))) {
      fail("Chose wrong rectangle");
      return PR_FALSE;
    }
    return PR_TRUE;
  }
  static PRBool TestContainsSpecifiedOverflowingRect() {
    nsRegion r(nsRect(0, 0, 100, 100));
    r.Or(r, nsRect(0, 300, 50, 50));
    if (!r.GetLargestRectangle(nsRect(0, 290, 10, 20)).IsEqualInterior(nsRect(0, 300, 50, 50))) {
      fail("Chose wrong rectangle");
      return PR_FALSE;
    }
    return PR_TRUE;
  }
public:
  static PRBool Test() {
    if (!TestSingleRect(nsRect(0, 52, 720, 480)) ||
        !TestSingleRect(nsRect(-20, 40, 50, 20)) ||
        !TestSingleRect(nsRect(-20, 40, 10, 8)) ||
        !TestSingleRect(nsRect(-20, -40, 10, 8)) ||
        !TestSingleRect(nsRect(-10, -10, 20, 20)))
      return PR_FALSE;
    if (!TestNonRectangular())
      return PR_FALSE;
    if (!TwoRectTest())
      return PR_FALSE;
    if (!TestContainsSpecifiedRect())
      return PR_FALSE;
    if (!TestContainsSpecifiedOverflowingRect())
      return PR_FALSE;
    passed("TestLargestRegion");
    return PR_TRUE;
  }
};

int main(int argc, char** argv) {
  ScopedXPCOM xpcom("TestRegion");
  if (xpcom.failed())
    return -1;
  if (!TestLargestRegion::Test())
    return -1;
  return 0;
}
