



#include "chrome/common/property_bag.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(PropertyBagTest, AddQueryRemove) {
  PropertyBag bag;
  PropertyAccessor<int> adaptor;

  
  EXPECT_EQ(NULL, adaptor.GetProperty(&bag));

  
  const int kFirstValue = 1;
  adaptor.SetProperty(&bag, kFirstValue);
  ASSERT_TRUE(adaptor.GetProperty(&bag));
  EXPECT_EQ(kFirstValue, *adaptor.GetProperty(&bag));

  
  const int kSecondValue = 2;
  adaptor.SetProperty(&bag, kSecondValue);
  ASSERT_TRUE(adaptor.GetProperty(&bag));
  EXPECT_EQ(kSecondValue, *adaptor.GetProperty(&bag));

  
  adaptor.DeleteProperty(&bag);
  EXPECT_EQ(NULL, adaptor.GetProperty(&bag));
}

TEST(PropertyBagTest, Copy) {
  PropertyAccessor<int> adaptor1;
  PropertyAccessor<double> adaptor2;

  
  PropertyBag copy;
  adaptor1.SetProperty(&copy, 22);

  const int kType1Value = 10;
  const double kType2Value = 2.7;
  {
    
    PropertyBag initial;
    adaptor1.SetProperty(&initial, kType1Value);
    adaptor2.SetProperty(&initial, kType2Value);

    
    copy = initial;
  }

  
  ASSERT_TRUE(adaptor1.GetProperty(&copy));
  ASSERT_TRUE(adaptor2.GetProperty(&copy));
  EXPECT_EQ(kType1Value, *adaptor1.GetProperty(&copy));
  EXPECT_EQ(kType2Value, *adaptor2.GetProperty(&copy));

  
  copy = PropertyBag();
  EXPECT_EQ(NULL, adaptor1.GetProperty(&copy));
  EXPECT_EQ(NULL, adaptor2.GetProperty(&copy));
}
