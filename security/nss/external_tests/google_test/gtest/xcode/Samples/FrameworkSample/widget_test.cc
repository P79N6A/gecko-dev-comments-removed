




































#include <string>
#include "gtest/gtest.h"

#include <Widget/widget.h>



TEST(WidgetInitializerTest, TestConstructor) {
  Widget widget(1.0f, "name");
  EXPECT_FLOAT_EQ(1.0f, widget.GetFloatValue());
  EXPECT_EQ(std::string("name"), widget.GetStringValue());
}



TEST(WidgetInitializerTest, TestConversion) {
  Widget widget(1.0f, "name");
  EXPECT_EQ(1, widget.GetIntValue());

  size_t max_size = 128;
  char buffer[max_size];
  widget.GetCharPtrValue(buffer, max_size);
  EXPECT_STREQ("name", buffer);
}







