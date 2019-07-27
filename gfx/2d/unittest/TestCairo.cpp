



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

TEST(Cairo, Bug1063486) {

  double x1, y1, x2, y2;
  const double epsilon = .01;

  cairo_surface_t *surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
  ASSERT_TRUE(surf != nullptr);

  cairo_t *cairo = cairo_create(surf);
  ASSERT_TRUE(cairo != nullptr);

  printf("Path 1\n");
  cairo_move_to(cairo, -20, -10);
  cairo_line_to(cairo, 20, -10);
  cairo_line_to(cairo, 20, 10);
  cairo_curve_to(cairo, 10,10, -10,10, -20,10);
  cairo_curve_to(cairo, -30,10, -30,-10, -20,-10);

  cairo_path_extents(cairo, &x1, &y1, &x2, &y2);

  ASSERT_LT(std::abs(-27.5 - x1), epsilon); 
  ASSERT_LT(std::abs(-10 - y1), epsilon);
  ASSERT_LT(std::abs(20 - x2), epsilon);
  ASSERT_LT(std::abs(10 - y2), epsilon);

  printf("Path 2\n");
  cairo_new_path(cairo);
  cairo_move_to(cairo, 10, 30);
  cairo_line_to(cairo, 90, 30);
  cairo_curve_to(cairo, 30,30, 30,30, 10,30);
  cairo_curve_to(cairo, 0,30, 0,0, 30,5);

  cairo_path_extents(cairo, &x1, &y1, &x2, &y2);

  ASSERT_LT(std::abs(4.019531 - x1), epsilon); 
  ASSERT_LT(std::abs(4.437500 - y1), epsilon);
  ASSERT_LT(std::abs(90. - x2), epsilon);
  ASSERT_LT(std::abs(30. - y2), epsilon);

  cairo_surface_destroy(surf);
  cairo_destroy(cairo);
}

} 
} 
