




































#include "nsRect.h"
#include <stdio.h>
#ifdef XP_WIN
#include <windows.h>
#endif

static PRBool
TestConstructors()
{
  
  nsRect  rect1(10, 20, 30, 40);

  
  if ((rect1.x != 10) || (rect1.y != 20) ||
      (rect1.width != 30) || (rect1.height != 40)) {
    printf("rect initialization failed!\n");
    return PR_FALSE;
  }

  
  nsRect  rect2(rect1);

  
  if ((rect2.x != rect1.x) || (rect2.y != rect1.y) ||
      (rect2.width != rect1.width) || (rect2.height != rect1.height)) {
    printf("rect copy constructor failed!\n");
    return PR_FALSE;
  }

  return PR_TRUE;
}

static PRBool
TestEqualityOperator()
{
  nsRect  rect1(10, 20, 30, 40);
  nsRect  rect2(rect1);

  
  if (!(rect1 == rect2)) {
    printf("rect equality operator failed!\n");
    return PR_FALSE;
  }

  
  if (rect1 != rect2) {
    printf("rect inequality operator failed!\n");
    return PR_FALSE;
  }

  
  rect1.Empty();
  rect2.Empty();
  if (!(rect1 == rect2)) {
    printf("rect equality operator failed for empty rects!\n");
    return PR_FALSE;
  }

  return PR_TRUE;
}

static PRBool
TestContainment()
{
  nsRect  rect1(10, 10, 50, 50);

  
  

  
  if (!rect1.Contains(rect1.x + rect1.width/2, rect1.y + rect1.height/2)) {
    printf("point containment test #1 failed!\n");
    return PR_FALSE;
  }

  
  if (!rect1.Contains(rect1.x, rect1.y)) {
    printf("point containment test #2 failed!\n");
    return PR_FALSE;
  }

  
  if (rect1.Contains(rect1.XMost(), rect1.YMost())) {
    printf("point containment test #3 failed!\n");
    return PR_FALSE;
  }

  
  
  nsRect  rect2(rect1);

  
  if (!rect1.Contains(rect2)) {
    printf("rect containment test #1 failed!\n");
    return PR_FALSE;
  }

  
  rect2.x--;
  if (rect1.Contains(rect2)) {
    printf("rect containment test #2 failed!\n");
    return PR_FALSE;
  }
  rect2.x++;

  
  rect2.y--;
  if (rect1.Contains(rect2)) {
    printf("rect containment test #3 failed!\n");
    return PR_FALSE;
  }
  rect2.y++;

  
  rect2.x++;
  if (rect1.Contains(rect2)) {
    printf("rect containment test #2 failed!\n");
    return PR_FALSE;
  }
  rect2.x--;

  
  rect2.y++;
  if (rect1.Contains(rect2)) {
    printf("rect containment test #3 failed!\n");
    return PR_FALSE;
  }
  rect2.y--;

  return PR_TRUE;
}



static PRBool
TestIntersects()
{
  nsRect  rect1(10, 10, 50, 50);
  nsRect  rect2(rect1);

  
  if (!rect1.Intersects(rect2)) {
    printf("rect intersects test #1 failed!\n");
    return PR_FALSE;
  }

  
  rect2.Deflate(1, 1);
  if (!rect1.Contains(rect2) || !rect1.Intersects(rect2)) {
    printf("rect intersects test #2 failed!\n");
    return PR_FALSE;
  }
  rect2.Inflate(1, 1);

  
  if (rect1 != rect2) {
    printf("rect inflate or deflate failed!\n");
    return PR_FALSE;
  }

  
  rect2.x--;
  if (!rect1.Intersects(rect2)) {
    printf("rect containment test #3 failed!\n");
    return PR_FALSE;
  }
  rect2.x++;

  
  rect2.x -= rect2.width;
  if (rect1.Intersects(rect2)) {
    printf("rect containment test #4 failed!\n");
    return PR_FALSE;
  }
  rect2.x += rect2.width;

  
  rect2.y--;
  if (!rect1.Intersects(rect2)) {
    printf("rect containment test #5 failed!\n");
    return PR_FALSE;
  }
  rect2.y++;

  
  rect2.y -= rect2.height;
  if (rect1.Intersects(rect2)) {
    printf("rect containment test #6 failed!\n");
    return PR_FALSE;
  }
  rect2.y += rect2.height;

  
  rect2.x++;
  if (!rect1.Intersects(rect2)) {
    printf("rect containment test #7 failed!\n");
    return PR_FALSE;
  }
  rect2.x--;

  
  rect2.x += rect2.width;
  if (rect1.Intersects(rect2)) {
    printf("rect containment test #8 failed!\n");
    return PR_FALSE;
  }
  rect2.x -= rect2.width;

  
  rect2.y++;
  if (!rect1.Intersects(rect2)) {
    printf("rect containment test #9 failed!\n");
    return PR_FALSE;
  }
  rect2.y--;

  
  rect2.y += rect2.height;
  if (rect1.Intersects(rect2)) {
    printf("rect containment test #10 failed!\n");
    return PR_FALSE;
  }
  rect2.y -= rect2.height;

  return PR_TRUE;
}


static PRBool
TestIntersection()
{
  nsRect  rect1(10, 10, 50, 50);
  nsRect  rect2(rect1);
  nsRect  dest;

  
  if (!dest.IntersectRect(rect1, rect2) || (dest != rect1)) {
    printf("rect intersection test #1 failed!\n");
    return PR_FALSE;
  }

  
  rect2.Deflate(1, 1);
  if (!dest.IntersectRect(rect1, rect2) || (dest != rect2)) {
    printf("rect intersection test #2 failed!\n");
    return PR_FALSE;
  }
  rect2.Inflate(1, 1);

  
  rect2.x--;
  if (!dest.IntersectRect(rect1, rect2) ||
     (dest != nsRect(rect1.x, rect1.y, rect1.width - 1, rect1.height))) {
    printf("rect intersection test #3 failed!\n");
    return PR_FALSE;
  }
  rect2.x++;

  
  rect2.x -= rect2.width;
  if (dest.IntersectRect(rect1, rect2)) {
    printf("rect intersection test #4 failed!\n");
    return PR_FALSE;
  }
  
  if (!dest.IsEmpty()) {
    printf("rect intersection test #4 no empty rect!\n");
    return PR_FALSE;
  }
  rect2.x += rect2.width;

  
  rect2.y--;
  if (!dest.IntersectRect(rect1, rect2) ||
     (dest != nsRect(rect1.x, rect1.y, rect1.width, rect1.height - 1))) {
    printf("rect intersection test #5 failed!\n");
    return PR_FALSE;
  }
  rect2.y++;

  
  rect2.y -= rect2.height;
  if (dest.IntersectRect(rect1, rect2)) {
    printf("rect intersection test #6 failed!\n");
    return PR_FALSE;
  }
  
  if (!dest.IsEmpty()) {
    printf("rect intersection test #6 no empty rect!\n");
    return PR_FALSE;
  }
  rect2.y += rect2.height;

  
  rect2.x++;
  if (!dest.IntersectRect(rect1, rect2) ||
     (dest != nsRect(rect1.x + 1, rect1.y, rect1.width - 1, rect1.height))) {
    printf("rect intersection test #7 failed!\n");
    return PR_FALSE;
  }
  rect2.x--;

  
  rect2.x += rect2.width;
  if (dest.IntersectRect(rect1, rect2)) {
    printf("rect intersection test #8 failed!\n");
    return PR_FALSE;
  }
  
  if (!dest.IsEmpty()) {
    printf("rect intersection test #8 no empty rect!\n");
    return PR_FALSE;
  }
  rect2.x -= rect2.width;

  
  rect2.y++;
  if (!dest.IntersectRect(rect1, rect2) ||
     (dest != nsRect(rect1.x, rect1.y + 1, rect1.width, rect1.height - 1))) {
    printf("rect intersection test #9 failed!\n");
    return PR_FALSE;
  }
  rect2.y--;

  
  rect2.y += rect2.height;
  if (dest.IntersectRect(rect1, rect2)) {
    printf("rect intersection test #10 failed!\n");
    return PR_FALSE;
  }
  
  if (!dest.IsEmpty()) {
    printf("rect intersection test #10 no empty rect!\n");
    return PR_FALSE;
  }
  rect2.y -= rect2.height;

  return PR_TRUE;
}

static PRBool
TestUnion()
{
  nsRect  rect1;
  nsRect  rect2(10, 10, 50, 50);
  nsRect  dest;

  
  rect1.Empty();
  if (!dest.UnionRect(rect1, rect2) || (dest != rect2)) {
    printf("rect union test #1 failed!\n");
    return PR_FALSE;
  }

  
  rect1 = rect2;
  rect2.Empty();
  if (!dest.UnionRect(rect1, rect2) || (dest != rect1)) {
    printf("rect union test #2 failed!\n");
    return PR_FALSE;
  }

  
  rect1.Empty();
  rect2.Empty();
  if (dest.UnionRect(rect1, rect2)) {
    printf("rect union test #3 failed!\n");
    return PR_FALSE;
  }

  
  rect1.SetRect(10, 10, 50, 50);
  rect2.SetRect(100, 100, 50, 50);
  if (!dest.UnionRect(rect1, rect2) ||
     (dest != nsRect(rect1.x, rect1.y, rect2.XMost() - rect1.x, rect2.YMost() - rect1.y))) {
    printf("rect union test #4 failed!\n");
    return PR_FALSE;
  }

  
  rect1.SetRect(30, 30, 50, 50);
  rect2.SetRect(10, 10, 50, 50);
  if (!dest.UnionRect(rect1, rect2) ||
      (dest != nsRect(rect2.x, rect2.y, rect1.XMost() - rect2.x, rect1.YMost() - rect2.y))) {
    printf("rect union test #5 failed!\n");
    return PR_FALSE;
  }

  return PR_TRUE;
}

int main(int argc, char** argv)
{
  if (!TestConstructors())
    return -1;

  if (!TestEqualityOperator())
    return -1;

  if (!TestContainment())
    return -1;

  if (!TestIntersects())
    return -1;

  if (!TestIntersection())
    return -1;

  if (!TestUnion())
    return -1;

  return 0;
}
