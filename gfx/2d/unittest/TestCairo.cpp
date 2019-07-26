



#include "cairo.h"

#include "gtest/gtest.h"

namespace mozilla {
namespace layers {

void TryCircle(double centerX, double centerY, double radius) {
  printf("TestCairo:TryArcs centerY %f, radius %f\n",centerY,radius);

  cairo_surface_t *surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,8,21);
  ASSERT_TRUE(surf != nullptr);

  cairo_t *cairo = cairo_create(surf);
  ASSERT_TRUE(cairo != nullptr);

  cairo_set_antialias(cairo, CAIRO_ANTIALIAS_NONE);
  cairo_arc(cairo, 0.0, centerY, radius, 0.0, 6.2831853071795862);
  cairo_fill_preserve(cairo);

  cairo_surface_destroy(surf);
  cairo_destroy(cairo);
}

TEST(Cairo, Simple) {
  TryCircle(0.0, 0.0, 14.0);
  TryCircle(0.0, 1.0, 22.4);
  TryCircle(1.0, 0.0, 1422.4);
  TryCircle(1.0, 1.0, 3422.4);
  TryCircle(-10.0, 1.0, -2);
}

TEST(Cairo, Bug825721) {
  
  TryCircle(0.0, 0.0, 8761126469220696064.0);
  TryCircle(0.0, 1.0, 8761126469220696064.0);

  
  TryCircle(1.0, 0.0, 5761126469220696064.0);

  
  
  
  
  TryCircle(0.0, 1.0, 5761126469220696064.0);
}

}
}
