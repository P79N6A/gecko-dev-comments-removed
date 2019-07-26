









#include "webrtc/modules/audio_coding/neteq4/audio_vector.h"

#include <assert.h>
#include <stdlib.h>

#include <string>

#include "gtest/gtest.h"
#include "webrtc/typedefs.h"

namespace webrtc {










template<typename T>
class AudioVectorTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    
    for (size_t i = 0; i < kLength; ++i) {
      array_[i] = static_cast<T>(i);
    }
  }

  enum {
    kLength = 10
  };

  T array_[kLength];
};


typedef ::testing::Types<int16_t, int32_t, double> MyTypes;
TYPED_TEST_CASE(AudioVectorTest, MyTypes);



TYPED_TEST(AudioVectorTest, CreateAndDestroy) {
  AudioVector<TypeParam> vec1;
  EXPECT_TRUE(vec1.Empty());
  EXPECT_EQ(0u, vec1.Size());

  size_t initial_size = 17;
  AudioVector<TypeParam> vec2(initial_size);
  EXPECT_FALSE(vec2.Empty());
  EXPECT_EQ(initial_size, vec2.Size());
}


TYPED_TEST(AudioVectorTest, SubscriptOperator) {
  AudioVector<TypeParam> vec(TestFixture::kLength);
  for (size_t i = 0; i < TestFixture::kLength; ++i) {
    vec[i] = static_cast<TypeParam>(i);
    const TypeParam& value = vec[i];  
    EXPECT_EQ(static_cast<TypeParam>(i), value);
  }
}



TYPED_TEST(AudioVectorTest, PushBackAndCopy) {
  AudioVector<TypeParam> vec;
  AudioVector<TypeParam> vec_copy;
  vec.PushBack(this->array_, TestFixture::kLength);
  vec.CopyFrom(&vec_copy);  
  ASSERT_EQ(TestFixture::kLength, vec.Size());
  ASSERT_EQ(TestFixture::kLength, vec_copy.Size());
  for (size_t i = 0; i < TestFixture::kLength; ++i) {
    EXPECT_EQ(this->array_[i], vec[i]);
    EXPECT_EQ(this->array_[i], vec_copy[i]);
  }

  
  vec.Clear();
  EXPECT_TRUE(vec.Empty());

  
  vec.CopyFrom(&vec_copy);
  EXPECT_TRUE(vec_copy.Empty());
}


TYPED_TEST(AudioVectorTest, CopyToNull) {
  AudioVector<TypeParam> vec;
  AudioVector<TypeParam>* vec_copy = NULL;
  vec.PushBack(this->array_, TestFixture::kLength);
  vec.CopyFrom(vec_copy);
}


TYPED_TEST(AudioVectorTest, PushBackVector) {
  static const size_t kLength = 10;
  AudioVector<TypeParam> vec1(kLength);
  AudioVector<TypeParam> vec2(kLength);
  
  
  for (size_t i = 0; i < kLength; ++i) {
    vec1[i] = static_cast<TypeParam>(i);
    vec2[i] = static_cast<TypeParam>(i + kLength);
  }
  
  vec1.PushBack(vec2);
  ASSERT_EQ(2 * kLength, vec1.Size());
  for (size_t i = 0; i < 2 * kLength; ++i) {
    EXPECT_EQ(static_cast<TypeParam>(i), vec1[i]);
  }
}


TYPED_TEST(AudioVectorTest, PushFront) {
  AudioVector<TypeParam> vec;
  vec.PushFront(this->array_, TestFixture::kLength);
  ASSERT_EQ(TestFixture::kLength, vec.Size());
  for (size_t i = 0; i < TestFixture::kLength; ++i) {
    EXPECT_EQ(this->array_[i], vec[i]);
  }
}


TYPED_TEST(AudioVectorTest, PushFrontVector) {
  static const size_t kLength = 10;
  AudioVector<TypeParam> vec1(kLength);
  AudioVector<TypeParam> vec2(kLength);
  
  
  for (size_t i = 0; i < kLength; ++i) {
    vec1[i] = static_cast<TypeParam>(i);
    vec2[i] = static_cast<TypeParam>(i + kLength);
  }
  
  vec2.PushFront(vec1);
  ASSERT_EQ(2 * kLength, vec2.Size());
  for (size_t i = 0; i < 2 * kLength; ++i) {
    EXPECT_EQ(static_cast<TypeParam>(i), vec2[i]);
  }
}


TYPED_TEST(AudioVectorTest, PopFront) {
  AudioVector<TypeParam> vec;
  vec.PushBack(this->array_, TestFixture::kLength);
  vec.PopFront(1);  
  EXPECT_EQ(TestFixture::kLength - 1u, vec.Size());
  for (size_t i = 0; i < TestFixture::kLength - 1; ++i) {
    EXPECT_EQ(static_cast<TypeParam>(i + 1), vec[i]);
  }
  vec.PopFront(TestFixture::kLength);  
  EXPECT_EQ(0u, vec.Size());
}


TYPED_TEST(AudioVectorTest, PopBack) {
  AudioVector<TypeParam> vec;
  vec.PushBack(this->array_, TestFixture::kLength);
  vec.PopBack(1);  
  EXPECT_EQ(TestFixture::kLength - 1u, vec.Size());
  for (size_t i = 0; i < TestFixture::kLength - 1; ++i) {
    EXPECT_EQ(static_cast<TypeParam>(i), vec[i]);
  }
  vec.PopBack(TestFixture::kLength);  
  EXPECT_EQ(0u, vec.Size());
}


TYPED_TEST(AudioVectorTest, Extend) {
  AudioVector<TypeParam> vec;
  vec.PushBack(this->array_, TestFixture::kLength);
  vec.Extend(5);  
  ASSERT_EQ(TestFixture::kLength + 5u, vec.Size());
  
  for (int i = TestFixture::kLength; i < TestFixture::kLength + 5; ++i) {
    EXPECT_EQ(0, vec[i]);
  }
}


TYPED_TEST(AudioVectorTest, InsertAt) {
  AudioVector<TypeParam> vec;
  vec.PushBack(this->array_, TestFixture::kLength);
  static const int kNewLength = 5;
  TypeParam new_array[kNewLength];
  
  for (int i = 0; i < kNewLength; ++i) {
    new_array[i] = 100 + i;
  }
  int insert_position = 5;
  vec.InsertAt(new_array, kNewLength, insert_position);
  
  
  
  int pos = 0;
  for (int i = 0; i < insert_position; ++i) {
    EXPECT_EQ(this->array_[i], vec[pos]);
    ++pos;
  }
  for (int i = 0; i < kNewLength; ++i) {
    EXPECT_EQ(new_array[i], vec[pos]);
    ++pos;
  }
  for (int i = insert_position; i < TestFixture::kLength; ++i) {
    EXPECT_EQ(this->array_[i], vec[pos]);
    ++pos;
  }
}



TYPED_TEST(AudioVectorTest, InsertZerosAt) {
  AudioVector<TypeParam> vec;
  AudioVector<TypeParam> vec_ref;
  vec.PushBack(this->array_, TestFixture::kLength);
  vec_ref.PushBack(this->array_, TestFixture::kLength);
  static const int kNewLength = 5;
  int insert_position = 5;
  vec.InsertZerosAt(kNewLength, insert_position);
  TypeParam new_array[kNewLength] = {0};  
  vec_ref.InsertAt(new_array, kNewLength, insert_position);
  
  ASSERT_EQ(vec_ref.Size(), vec.Size());
  for (size_t i = 0; i < vec.Size(); ++i) {
    EXPECT_EQ(vec_ref[i], vec[i]);
  }
}


TYPED_TEST(AudioVectorTest, InsertAtBeginning) {
  AudioVector<TypeParam> vec;
  vec.PushBack(this->array_, TestFixture::kLength);
  static const int kNewLength = 5;
  TypeParam new_array[kNewLength];
  
  for (int i = 0; i < kNewLength; ++i) {
    new_array[i] = 100 + i;
  }
  int insert_position = 0;
  vec.InsertAt(new_array, kNewLength, insert_position);
  
  
  
  int pos = 0;
  for (int i = 0; i < kNewLength; ++i) {
    EXPECT_EQ(new_array[i], vec[pos]);
    ++pos;
  }
  for (int i = insert_position; i < TestFixture::kLength; ++i) {
    EXPECT_EQ(this->array_[i], vec[pos]);
    ++pos;
  }
}


TYPED_TEST(AudioVectorTest, InsertAtEnd) {
  AudioVector<TypeParam> vec;
  vec.PushBack(this->array_, TestFixture::kLength);
  static const int kNewLength = 5;
  TypeParam new_array[kNewLength];
  
  for (int i = 0; i < kNewLength; ++i) {
    new_array[i] = 100 + i;
  }
  int insert_position = TestFixture::kLength;
  vec.InsertAt(new_array, kNewLength, insert_position);
  
  
  int pos = 0;
  for (int i = 0; i < TestFixture::kLength; ++i) {
    EXPECT_EQ(this->array_[i], vec[pos]);
    ++pos;
  }
  for (int i = 0; i < kNewLength; ++i) {
    EXPECT_EQ(new_array[i], vec[pos]);
    ++pos;
  }
}






TYPED_TEST(AudioVectorTest, InsertBeyondEnd) {
  AudioVector<TypeParam> vec;
  vec.PushBack(this->array_, TestFixture::kLength);
  static const int kNewLength = 5;
  TypeParam new_array[kNewLength];
  
  for (int i = 0; i < kNewLength; ++i) {
    new_array[i] = 100 + i;
  }
  int insert_position = TestFixture::kLength + 10;  
  vec.InsertAt(new_array, kNewLength, insert_position);
  
  
  int pos = 0;
  for (int i = 0; i < TestFixture::kLength; ++i) {
    EXPECT_EQ(this->array_[i], vec[pos]);
    ++pos;
  }
  for (int i = 0; i < kNewLength; ++i) {
    EXPECT_EQ(new_array[i], vec[pos]);
    ++pos;
  }
}



TYPED_TEST(AudioVectorTest, OverwriteAt) {
  AudioVector<TypeParam> vec;
  vec.PushBack(this->array_, TestFixture::kLength);
  static const int kNewLength = 5;
  TypeParam new_array[kNewLength];
  
  for (int i = 0; i < kNewLength; ++i) {
    new_array[i] = 100 + i;
  }
  int insert_position = 2;
  vec.OverwriteAt(new_array, kNewLength, insert_position);
  
  
  
  int pos = 0;
  for (pos = 0; pos < insert_position; ++pos) {
    EXPECT_EQ(this->array_[pos], vec[pos]);
  }
  for (int i = 0; i < kNewLength; ++i) {
    EXPECT_EQ(new_array[i], vec[pos]);
    ++pos;
  }
  for (; pos < TestFixture::kLength; ++pos) {
    EXPECT_EQ(this->array_[pos], vec[pos]);
  }
}




TYPED_TEST(AudioVectorTest, OverwriteBeyondEnd) {
  AudioVector<TypeParam> vec;
  vec.PushBack(this->array_, TestFixture::kLength);
  static const int kNewLength = 5;
  TypeParam new_array[kNewLength];
  
  for (int i = 0; i < kNewLength; ++i) {
    new_array[i] = 100 + i;
  }
  int insert_position = TestFixture::kLength - 2;
  vec.OverwriteAt(new_array, kNewLength, insert_position);
  ASSERT_EQ(TestFixture::kLength - 2u + kNewLength, vec.Size());
  
  
  
  int pos = 0;
  for (pos = 0; pos < insert_position; ++pos) {
    EXPECT_EQ(this->array_[pos], vec[pos]);
  }
  for (int i = 0; i < kNewLength; ++i) {
    EXPECT_EQ(new_array[i], vec[pos]);
    ++pos;
  }
  
  EXPECT_EQ(vec.Size(), static_cast<size_t>(pos));
}

TYPED_TEST(AudioVectorTest, CrossFade) {
  static const size_t kLength = 100;
  static const size_t kFadeLength = 10;
  AudioVector<TypeParam> vec1(kLength);
  AudioVector<TypeParam> vec2(kLength);
  
  for (size_t i = 0; i < kLength; ++i) {
    vec1[i] = 0;
    vec2[i] = 100;
  }
  vec1.CrossFade(vec2, kFadeLength);
  ASSERT_EQ(2 * kLength - kFadeLength, vec1.Size());
  
  for (size_t i = 0; i < kLength - kFadeLength; ++i) {
    EXPECT_EQ(0, vec1[i]);
  }
  
  for (size_t i = 0 ; i < kFadeLength; ++i) {
    EXPECT_NEAR((i + 1) * 100 / (kFadeLength + 1),
                vec1[kLength - kFadeLength + i], 1);
  }
  
  for (size_t i = kLength; i < vec1.Size(); ++i) {
    EXPECT_EQ(100, vec1[i]);
  }
}

}  
