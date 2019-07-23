



#include <string.h>

#include "chrome/common/ipc_message.h"
#include "chrome/common/ipc_message_utils.h"
#include "base/scoped_ptr.h"
#include "googleurl/src/gurl.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "SkBitmap.h"


TEST(IPCMessageTest, Serialize) {
  const char* serialize_cases[] = {
    "http://www.google.com/",
    "http://user:pass@host.com:888/foo;bar?baz#nop",
    "#inva://idurl/",
  };

  for (size_t i = 0; i < arraysize(serialize_cases); i++) {
    GURL input(serialize_cases[i]);
    IPC::Message msg(1, 2, IPC::Message::PRIORITY_NORMAL);
    IPC::ParamTraits<GURL>::Write(&msg, input);

    GURL output;
    void* iter = NULL;
    EXPECT_TRUE(IPC::ParamTraits<GURL>::Read(&msg, &iter, &output));

    
    
    EXPECT_EQ(input.possibly_invalid_spec(), output.possibly_invalid_spec());
    EXPECT_EQ(input.is_valid(), output.is_valid());
    EXPECT_EQ(input.scheme(), output.scheme());
    EXPECT_EQ(input.username(), output.username());
    EXPECT_EQ(input.password(), output.password());
    EXPECT_EQ(input.host(), output.host());
    EXPECT_EQ(input.port(), output.port());
    EXPECT_EQ(input.path(), output.path());
    EXPECT_EQ(input.query(), output.query());
    EXPECT_EQ(input.ref(), output.ref());
  }

  
  IPC::Message msg(1, 2, IPC::Message::PRIORITY_NORMAL);
  msg.WriteInt(99);
  GURL output;
  void* iter = NULL;
  EXPECT_FALSE(IPC::ParamTraits<GURL>::Read(&msg, &iter, &output));
}


TEST(IPCMessageTest, Bitmap) {
  SkBitmap bitmap;

  bitmap.setConfig(SkBitmap::kARGB_8888_Config, 10, 5);
  bitmap.allocPixels();
  memset(bitmap.getPixels(), 'A', bitmap.getSize());

  IPC::Message msg(1, 2, IPC::Message::PRIORITY_NORMAL);
  IPC::ParamTraits<SkBitmap>::Write(&msg, bitmap);

  SkBitmap output;
  void* iter = NULL;
  EXPECT_TRUE(IPC::ParamTraits<SkBitmap>::Read(&msg, &iter, &output));

  EXPECT_EQ(bitmap.config(), output.config());
  EXPECT_EQ(bitmap.width(), output.width());
  EXPECT_EQ(bitmap.height(), output.height());
  EXPECT_EQ(bitmap.rowBytes(), output.rowBytes());
  EXPECT_EQ(bitmap.getSize(), output.getSize());
  EXPECT_EQ(memcmp(bitmap.getPixels(), output.getPixels(), bitmap.getSize()),
            0);

  
  IPC::Message bad_msg(1, 2, IPC::Message::PRIORITY_NORMAL);
  
  const char* fixed_data;
  int fixed_data_size;
  iter = NULL;
  msg.ReadData(&iter, &fixed_data, &fixed_data_size);
  bad_msg.WriteData(fixed_data, fixed_data_size);
  
  const size_t bogus_pixels_size = bitmap.getSize() * 2;
  scoped_array<char> bogus_pixels(new char[bogus_pixels_size]);
  memset(bogus_pixels.get(), 'B', bogus_pixels_size);
  bad_msg.WriteData(bogus_pixels.get(), bogus_pixels_size);
  
  SkBitmap bad_output;
  iter = NULL;
  EXPECT_FALSE(IPC::ParamTraits<SkBitmap>::Read(&bad_msg, &iter, &bad_output));
}
