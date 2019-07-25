




































#include "TestHarness.h"

#include "nsRect.h"
#ifdef XP_WIN
#include <windows.h>
#endif

template <class RectType>
static bool
TestConstructors()
{
  
  RectType  rect1(10, 20, 30, 40);

  
  if ((rect1.x != 10) || (rect1.y != 20) ||
      (rect1.width != 30) || (rect1.height != 40)) {
    fail("[1] Make sure the rectangle was properly initialized with constructor");
    return false;
  }

  
  RectType  rect2(rect1);

  
  if ((rect2.x != rect1.x) || (rect2.y != rect1.y) ||
      (rect2.width != rect1.width) || (rect2.height != rect1.height)) {
    fail("[2] Make sure the rectangle was properly initialized with copy constructor");
    return false;
  }

  passed("TestConstructors");
  return true;
}

template <class RectType>
static bool
TestEqualityOperator()
{
  RectType  rect1(10, 20, 30, 40);
  RectType  rect2(rect1);

  
  if (!(rect1 == rect2)) {
    fail("[1] Test the equality operator");
    return false;
  }

  
  if (rect1 != rect2) {
    fail("[2] Test the inequality operator");
    return false;
  }

  
  rect1.Empty();
  rect2.Empty();
  if (!(rect1 == rect2)) {
    fail("[3] Make sure that two empty rects are equal");
    return false;
  }

  passed("TestEqualityOperator");
  return true;
}

template <class RectType>
static bool
TestContainment()
{
  RectType  rect1(10, 10, 50, 50);

  
  

  
  if (!rect1.Contains(rect1.x + rect1.width/2, rect1.y + rect1.height/2)) {
    fail("[1] Basic test of a point in the middle of the rect");
    return false;
  }

  
  if (!rect1.Contains(rect1.x, rect1.y)) {
    fail("[2] Test against a point at the left/top edges");
    return false;
  }

  
  if (rect1.Contains(rect1.XMost(), rect1.YMost())) {
    fail("[3] Test against a point at the right/bottom extents");
    return false;
  }

  
  
  RectType  rect2(rect1);

  
  if (!rect1.Contains(rect2)) {
    fail("[4] Test against a rect that's the same as rect1");
    return false;
  }

  
  rect2.x--;
  if (rect1.Contains(rect2)) {
    fail("[5] Test against a rect whose left edge (only) is outside of rect1");
    return false;
  }
  rect2.x++;

  
  rect2.y--;
  if (rect1.Contains(rect2)) {
    fail("[6] Test against a rect whose top edge (only) is outside of rect1");
    return false;
  }
  rect2.y++;

  
  rect2.x++;
  if (rect1.Contains(rect2)) {
    fail("[7] Test against a rect whose right edge (only) is outside of rect1");
    return false;
  }
  rect2.x--;

  
  rect2.y++;
  if (rect1.Contains(rect2)) {
    fail("[8] Test against a rect whose bottom edge (only) is outside of rect1");
    return false;
  }
  rect2.y--;

  passed("TestContainment");
  return true;
}



template <class RectType>
static bool
TestIntersects()
{
  RectType  rect1(10, 10, 50, 50);
  RectType  rect2(rect1);

  
  if (!rect1.Intersects(rect2)) {
    fail("[1] Test against a rect that's the same as rect1");
    return false;
  }

  
  rect2.Inflate(-1, -1);
  if (!rect1.Contains(rect2) || !rect1.Intersects(rect2)) {
    fail("[2] Test against a rect that's enclosed by rect1");
    return false;
  }
  rect2.Inflate(1, 1);

  
  if (rect1 != rect2) {
    fail("[3] Make sure inflate and deflate worked correctly");
    return false;
  }

  
  rect2.x--;
  if (!rect1.Intersects(rect2)) {
    fail("[4] Test against a rect that overlaps the left edge of rect1");
    return false;
  }
  rect2.x++;

  
  rect2.x -= rect2.width;
  if (rect1.Intersects(rect2)) {
    fail("[5] Test against a rect that's outside of rect1 on the left");
    return false;
  }
  rect2.x += rect2.width;

  
  rect2.y--;
  if (!rect1.Intersects(rect2)) {
    fail("[6] Test against a rect that overlaps the top edge of rect1");
    return false;
  }
  rect2.y++;

  
  rect2.y -= rect2.height;
  if (rect1.Intersects(rect2)) {
    fail("[7] Test against a rect that's outside of rect1 on the top");
    return false;
  }
  rect2.y += rect2.height;

  
  rect2.x++;
  if (!rect1.Intersects(rect2)) {
    fail("[8] Test against a rect that overlaps the right edge of rect1");
    return false;
  }
  rect2.x--;

  
  rect2.x += rect2.width;
  if (rect1.Intersects(rect2)) {
    fail("[9] Test against a rect that's outside of rect1 on the right");
    return false;
  }
  rect2.x -= rect2.width;

  
  rect2.y++;
  if (!rect1.Intersects(rect2)) {
    fail("[10] Test against a rect that overlaps the bottom edge of rect1");
    return false;
  }
  rect2.y--;

  
  rect2.y += rect2.height;
  if (rect1.Intersects(rect2)) {
    fail("[11] Test against a rect that's outside of rect1 on the bottom");
    return false;
  }
  rect2.y -= rect2.height;

  passed("TestIntersects");
  return true;
}


template <class RectType>
static bool
TestIntersection()
{
  RectType  rect1(10, 10, 50, 50);
  RectType  rect2(rect1);
  RectType  dest;

  
  if (!dest.IntersectRect(rect1, rect2) || (dest != rect1)) {
    fail("[1] Test against a rect that's the same as rect1");
    return false;
  }

  
  rect2.Inflate(-1, -1);
  if (!dest.IntersectRect(rect1, rect2) || (dest != rect2)) {
    fail("[2] Test against a rect that's enclosed by rect1");
    return false;
  }
  rect2.Inflate(1, 1);

  
  rect2.x--;
  if (!dest.IntersectRect(rect1, rect2) ||
     (dest != RectType(rect1.x, rect1.y, rect1.width - 1, rect1.height))) {
    fail("[3] Test against a rect that overlaps the left edge of rect1");
    return false;
  }
  rect2.x++;

  
  rect2.x -= rect2.width;
  if (dest.IntersectRect(rect1, rect2)) {
    fail("[4] Test against a rect that's outside of rect1 on the left");
    return false;
  }
  
  if (!dest.IsEmpty()) {
    fail("[4] Make sure an empty rect is returned");
    return false;
  }
  rect2.x += rect2.width;

  
  rect2.y--;
  if (!dest.IntersectRect(rect1, rect2) ||
     (dest != RectType(rect1.x, rect1.y, rect1.width, rect1.height - 1))) {
    fail("[5] Test against a rect that overlaps the top edge of rect1");
    return false;
  }
  rect2.y++;

  
  rect2.y -= rect2.height;
  if (dest.IntersectRect(rect1, rect2)) {
    fail("[6] Test against a rect that's outside of rect1 on the top");
    return false;
  }
  
  if (!dest.IsEmpty()) {
    fail("[6] Make sure an empty rect is returned");
    return false;
  }
  rect2.y += rect2.height;

  
  rect2.x++;
  if (!dest.IntersectRect(rect1, rect2) ||
     (dest != RectType(rect1.x + 1, rect1.y, rect1.width - 1, rect1.height))) {
    fail("[7] Test against a rect that overlaps the right edge of rect1");
    return false;
  }
  rect2.x--;

  
  rect2.x += rect2.width;
  if (dest.IntersectRect(rect1, rect2)) {
    fail("[8] Test against a rect that's outside of rect1 on the right");
    return false;
  }
  
  if (!dest.IsEmpty()) {
    fail("[8] Make sure an empty rect is returned");
    return false;
  }
  rect2.x -= rect2.width;

  
  rect2.y++;
  if (!dest.IntersectRect(rect1, rect2) ||
     (dest != RectType(rect1.x, rect1.y + 1, rect1.width, rect1.height - 1))) {
    fail("[9] Test against a rect that overlaps the bottom edge of rect1");
    return false;
  }
  rect2.y--;

  
  rect2.y += rect2.height;
  if (dest.IntersectRect(rect1, rect2)) {
    fail("[10] Test against a rect that's outside of rect1 on the bottom");
    return false;
  }
  
  if (!dest.IsEmpty()) {
    fail("[10] Make sure an empty rect is returned");
    return false;
  }
  rect2.y -= rect2.height;

  
  rect1.SetRect(100, 100, 100, 100);
  rect2.SetRect(150, 100, 0, 100);
  if (dest.IntersectRect(rect1, rect2) || !dest.IsEmpty()) {
    fail("[11] Intersection of rects with zero width or height should be empty");
    return false;
  }

  
  

  
  rect1.SetRect(100, 100, 100, 100);
  rect2.SetRect(100, 100, -100, 100);
  if (dest.IntersectRect(rect1, rect2) || !dest.IsEmpty()) {
    fail("[12] Intersection of rects with negative width or height should be empty");
    return false;
  }

  
  
  rect1.SetRect(100, 100, 100, 100);
  rect2.SetRect(200, 200, -100, -100);
  if (dest.IntersectRect(rect1, rect2) || !dest.IsEmpty()) {
    fail("[13] Intersection of rects with negative width or height should be empty");
    return false;
  }

  
  rect1.SetRect(100, 100, 100, -100);
  rect2.SetRect(100, 100, 100, -100);
  if (dest.IntersectRect(rect1, rect2) || !dest.IsEmpty()) {
    fail("[14] Intersection of rects with negative width or height should be empty");
    return false;
  }

  passed("TestIntersection");
  return true;
}

template <class RectType>
static bool
TestUnion()
{
  RectType  rect1;
  RectType  rect2(10, 10, 50, 50);
  RectType  dest;

  
  rect1.Empty();
  if (!dest.UnionRect(rect1, rect2) || (dest != rect2)) {
    fail("[1] Check the case where the receiver is an empty rect");
    return false;
  }

  
  rect1 = rect2;
  rect2.Empty();
  if (!dest.UnionRect(rect1, rect2) || (dest != rect1)) {
    fail("[2] Check the case where the source rect is an empty rect");
    return false;
  }

  
  rect1.Empty();
  rect2.Empty();
  if (dest.UnionRect(rect1, rect2)) {
    fail("[3] Test the case where both rects are empty");
    return false;
  }

  
  rect1.SetRect(10, 10, 50, 50);
  rect2.SetRect(100, 100, 50, 50);
  if (!dest.UnionRect(rect1, rect2) ||
     (dest != RectType(rect1.x, rect1.y, rect2.XMost() - rect1.x, rect2.YMost() - rect1.y))) {
    fail("[4] Test union case where the two rects don't overlap at all");
    return false;
  }

  
  rect1.SetRect(30, 30, 50, 50);
  rect2.SetRect(10, 10, 50, 50);
  if (!dest.UnionRect(rect1, rect2) ||
      (dest != RectType(rect2.x, rect2.y, rect1.XMost() - rect2.x, rect1.YMost() - rect2.y))) {
    fail("[5] Test union case where the two rects overlap");
    return false;
  }

  passed("TestUnion");
  return true;
}

int main(int argc, char** argv)
{
  ScopedXPCOM xpcom("TestRect");
  if (xpcom.failed())
    return -1;

  int rv = 0;

  
  
  
  printf("===== nsRect tests =====\n");

  if (!TestConstructors<nsRect>())
    rv = -1;

  if (!TestEqualityOperator<nsRect>())
    rv = -1;

  if (!TestContainment<nsRect>())
    rv = -1;

  if (!TestIntersects<nsRect>())
    rv = -1;

  if (!TestIntersection<nsRect>())
    rv = -1;

  if (!TestUnion<nsRect>())
    rv = -1;

  
  
  
  printf("===== nsIntRect tests =====\n");
 
  if (!TestConstructors<nsIntRect>())
    rv = -1;

  if (!TestEqualityOperator<nsIntRect>())
    rv = -1;

  if (!TestContainment<nsIntRect>())
    rv = -1;

  if (!TestIntersects<nsIntRect>())
    rv = -1;

  if (!TestIntersection<nsIntRect>())
    rv = -1;

  if (!TestUnion<nsIntRect>())
    rv = -1;

  return rv;
}
