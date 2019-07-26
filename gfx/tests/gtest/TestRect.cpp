




#include "gtest/gtest.h"

#include "nsRect.h"
#ifdef XP_WIN
#include <windows.h>
#endif

template <class RectType>
static bool
TestConstructors()
{
  
  RectType  rect1(10, 20, 30, 40);

  
  EXPECT_TRUE(rect1.x == 10 && rect1.y == 20 &&
      rect1.width == 30 && rect1.height == 40) <<
    "[1] Make sure the rectangle was properly initialized with constructor";

  
  RectType  rect2(rect1);

  
  EXPECT_TRUE(rect2.x == rect1.x && rect2.y == rect2.y &&
      rect2.width == rect2.width && rect2.height == rect2.height) <<
    "[2] Make sure the rectangle was properly initialized with copy constructor";

  return true;
}

template <class RectType>
static bool
TestEqualityOperator()
{
  RectType  rect1(10, 20, 30, 40);
  RectType  rect2(rect1);

  
  EXPECT_TRUE(rect1 == rect2) <<
    "[1] Test the equality operator";

  EXPECT_FALSE(!rect1.IsEqualInterior(rect2)) <<
    "[2] Test the inequality operator";

  
  rect1.SetEmpty();
  rect2.SetEmpty();
  EXPECT_TRUE(rect1 == rect2) <<
    "[3] Make sure that two empty rects are equal";

  return true;
}

template <class RectType>
static bool
TestContainment()
{
  RectType  rect1(10, 10, 50, 50);

  
  

  
  EXPECT_FALSE(!rect1.Contains(rect1.x + rect1.width/2, rect1.y + rect1.height/2)) <<
    "[1] Basic test of a point in the middle of the rect";

  
  EXPECT_FALSE(!rect1.Contains(rect1.x, rect1.y)) <<
    "[2] Test against a point at the left/top edges";

  
  EXPECT_FALSE(rect1.Contains(rect1.XMost(), rect1.YMost())) <<
    "[3] Test against a point at the right/bottom extents";

  
  
  RectType  rect2(rect1);

  
  EXPECT_FALSE(!rect1.Contains(rect2)) <<
    "[4] Test against a rect that's the same as rect1";

  
  rect2.x--;
  EXPECT_FALSE(rect1.Contains(rect2)) <<
    "[5] Test against a rect whose left edge (only) is outside of rect1";
  rect2.x++;

  
  rect2.y--;
  EXPECT_FALSE(rect1.Contains(rect2)) <<
    "[6] Test against a rect whose top edge (only) is outside of rect1";
  rect2.y++;

  
  rect2.x++;
  EXPECT_FALSE(rect1.Contains(rect2)) <<
    "[7] Test against a rect whose right edge (only) is outside of rect1";
  rect2.x--;

  
  rect2.y++;
  EXPECT_FALSE(rect1.Contains(rect2)) <<
    "[8] Test against a rect whose bottom edge (only) is outside of rect1";
  rect2.y--;

  return true;
}



template <class RectType>
static bool
TestIntersects()
{
  RectType  rect1(10, 10, 50, 50);
  RectType  rect2(rect1);

  
  EXPECT_FALSE(!rect1.Intersects(rect2)) <<
    "[1] Test against a rect that's the same as rect1";

  
  rect2.Inflate(-1, -1);
  EXPECT_FALSE(!rect1.Contains(rect2) || !rect1.Intersects(rect2)) <<
    "[2] Test against a rect that's enclosed by rect1";
  rect2.Inflate(1, 1);

  
  EXPECT_TRUE(rect1.IsEqualInterior(rect2)) <<
    "[3] Make sure inflate and deflate worked correctly";

  
  rect2.x--;
  EXPECT_FALSE(!rect1.Intersects(rect2)) <<
    "[4] Test against a rect that overlaps the left edge of rect1";
  rect2.x++;

  
  rect2.x -= rect2.width;
  EXPECT_FALSE(rect1.Intersects(rect2)) <<
    "[5] Test against a rect that's outside of rect1 on the left";
  rect2.x += rect2.width;

  
  rect2.y--;
  EXPECT_FALSE(!rect1.Intersects(rect2)) <<
    "[6] Test against a rect that overlaps the top edge of rect1";
  rect2.y++;

  
  rect2.y -= rect2.height;
  EXPECT_FALSE(rect1.Intersects(rect2)) <<
    "[7] Test against a rect that's outside of rect1 on the top";
  rect2.y += rect2.height;

  
  rect2.x++;
  EXPECT_FALSE(!rect1.Intersects(rect2)) <<
    "[8] Test against a rect that overlaps the right edge of rect1";
  rect2.x--;

  
  rect2.x += rect2.width;
  EXPECT_FALSE(rect1.Intersects(rect2)) <<
    "[9] Test against a rect that's outside of rect1 on the right";
  rect2.x -= rect2.width;

  
  rect2.y++;
  EXPECT_FALSE(!rect1.Intersects(rect2)) <<
    "[10] Test against a rect that overlaps the bottom edge of rect1";
  rect2.y--;

  
  rect2.y += rect2.height;
  EXPECT_FALSE(rect1.Intersects(rect2)) <<
    "[11] Test against a rect that's outside of rect1 on the bottom";
  rect2.y -= rect2.height;

  return true;
}


template <class RectType>
static bool
TestIntersection()
{
  RectType  rect1(10, 10, 50, 50);
  RectType  rect2(rect1);
  RectType  dest;

  
  EXPECT_FALSE(!dest.IntersectRect(rect1, rect2) || !(dest.IsEqualInterior(rect1))) <<
    "[1] Test against a rect that's the same as rect1";

  
  rect2.Inflate(-1, -1);
  EXPECT_FALSE(!dest.IntersectRect(rect1, rect2) || !(dest.IsEqualInterior(rect2))) <<
    "[2] Test against a rect that's enclosed by rect1";
  rect2.Inflate(1, 1);

  
  rect2.x--;
  EXPECT_FALSE(!dest.IntersectRect(rect1, rect2) ||
     !(dest.IsEqualInterior(RectType(rect1.x, rect1.y, rect1.width - 1, rect1.height)))) <<
    "[3] Test against a rect that overlaps the left edge of rect1";
  rect2.x++;

  
  rect2.x -= rect2.width;
  EXPECT_FALSE(dest.IntersectRect(rect1, rect2)) <<
    "[4] Test against a rect that's outside of rect1 on the left";
  
  EXPECT_FALSE(!dest.IsEmpty()) <<
    "[4] Make sure an empty rect is returned";
  rect2.x += rect2.width;

  
  rect2.y--;
  EXPECT_FALSE(!dest.IntersectRect(rect1, rect2) ||
     !(dest.IsEqualInterior(RectType(rect1.x, rect1.y, rect1.width, rect1.height - 1)))) <<
    "[5] Test against a rect that overlaps the top edge of rect1";
  rect2.y++;

  
  rect2.y -= rect2.height;
  EXPECT_FALSE(dest.IntersectRect(rect1, rect2)) <<
    "[6] Test against a rect that's outside of rect1 on the top";
  
  EXPECT_FALSE(!dest.IsEmpty()) <<
    "[6] Make sure an empty rect is returned";
  rect2.y += rect2.height;

  
  rect2.x++;
  EXPECT_FALSE(!dest.IntersectRect(rect1, rect2) ||
     !(dest.IsEqualInterior(RectType(rect1.x + 1, rect1.y, rect1.width - 1, rect1.height)))) <<
    "[7] Test against a rect that overlaps the right edge of rect1";
  rect2.x--;

  
  rect2.x += rect2.width;
  EXPECT_FALSE(dest.IntersectRect(rect1, rect2)) <<
    "[8] Test against a rect that's outside of rect1 on the right";
  
  EXPECT_FALSE(!dest.IsEmpty()) <<
    "[8] Make sure an empty rect is returned";
  rect2.x -= rect2.width;

  
  rect2.y++;
  EXPECT_FALSE(!dest.IntersectRect(rect1, rect2) ||
     !(dest.IsEqualInterior(RectType(rect1.x, rect1.y + 1, rect1.width, rect1.height - 1)))) <<
    "[9] Test against a rect that overlaps the bottom edge of rect1";
  rect2.y--;

  
  rect2.y += rect2.height;
  EXPECT_FALSE(dest.IntersectRect(rect1, rect2)) <<
    "[10] Test against a rect that's outside of rect1 on the bottom";
  
  EXPECT_FALSE(!dest.IsEmpty()) <<
    "[10] Make sure an empty rect is returned";
  rect2.y -= rect2.height;

  
  rect1.SetRect(100, 100, 100, 100);
  rect2.SetRect(150, 100, 0, 100);
  EXPECT_FALSE(dest.IntersectRect(rect1, rect2) || !dest.IsEmpty()) <<
    "[11] Intersection of rects with zero width or height should be empty";

  
  

  
  rect1.SetRect(100, 100, 100, 100);
  rect2.SetRect(100, 100, -100, 100);
  EXPECT_FALSE(dest.IntersectRect(rect1, rect2) || !dest.IsEmpty()) <<
    "[12] Intersection of rects with negative width or height should be empty";

  
  
  rect1.SetRect(100, 100, 100, 100);
  rect2.SetRect(200, 200, -100, -100);
  EXPECT_FALSE(dest.IntersectRect(rect1, rect2) || !dest.IsEmpty()) <<
    "[13] Intersection of rects with negative width or height should be empty";

  
  rect1.SetRect(100, 100, 100, -100);
  rect2.SetRect(100, 100, 100, -100);
  EXPECT_FALSE(dest.IntersectRect(rect1, rect2) || !dest.IsEmpty()) <<
    "[14] Intersection of rects with negative width or height should be empty";

  return true;
}

template <class RectType>
static bool
TestUnion()
{
  RectType  rect1;
  RectType  rect2(10, 10, 50, 50);
  RectType  dest;

  
  rect1.SetEmpty();
  dest.UnionRect(rect1, rect2);
  EXPECT_FALSE(dest.IsEmpty() || !dest.IsEqualInterior(rect2)) <<
    "[1] Check the case where the receiver is an empty rect";

  
  rect1 = rect2;
  rect2.SetEmpty();
  dest.UnionRect(rect1, rect2);
  EXPECT_FALSE(dest.IsEmpty() || !dest.IsEqualInterior(rect1)) <<
    "[2] Check the case where the source rect is an empty rect";

  
  rect1.SetEmpty();
  rect2.SetEmpty();
  dest.UnionRect(rect1, rect2);
  EXPECT_FALSE(!dest.IsEmpty()) <<
    "[3] Test the case where both rects are empty";

  
  rect1.SetRect(10, 10, 50, 50);
  rect2.SetRect(100, 100, 50, 50);
  dest.UnionRect(rect1, rect2);
  EXPECT_FALSE(dest.IsEmpty() ||
     !(dest.IsEqualInterior(RectType(rect1.x, rect1.y, rect2.XMost() - rect1.x, rect2.YMost() - rect1.y)))) <<
    "[4] Test union case where the two rects don't overlap at all";

  
  rect1.SetRect(30, 30, 50, 50);
  rect2.SetRect(10, 10, 50, 50);
  dest.UnionRect(rect1, rect2);
  EXPECT_FALSE(dest.IsEmpty() ||
      !(dest.IsEqualInterior(RectType(rect2.x, rect2.y, rect1.XMost() - rect2.x, rect1.YMost() - rect2.y)))) <<
    "[5] Test union case where the two rects overlap";

  return true;
}

TEST(Gfx, nsRect) {
  TestConstructors<nsRect>();
  TestEqualityOperator<nsRect>();
  TestContainment<nsRect>();
  TestIntersects<nsRect>();
  TestIntersection<nsRect>();
  TestUnion<nsRect>();
}

TEST(Gfx, nsIntRect) {
  TestConstructors<nsIntRect>();
  TestEqualityOperator<nsIntRect>();
  TestContainment<nsIntRect>();
  TestIntersects<nsIntRect>();
  TestIntersection<nsIntRect>();
  TestUnion<nsIntRect>();
}

