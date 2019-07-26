









#include "webrtc/modules/audio_coding/neteq4/audio_multi_vector.h"

#include <assert.h>
#include <stdlib.h>

#include <string>

#include "gtest/gtest.h"
#include "webrtc/typedefs.h"

namespace webrtc {








class AudioMultiVectorTest : public ::testing::TestWithParam<size_t> {
 protected:
  typedef int16_t T;  

  AudioMultiVectorTest()
      : num_channels_(GetParam()),  
        interleaved_length_(num_channels_ * kLength) {
    array_interleaved_ = new T[num_channels_ * kLength];
  }

  ~AudioMultiVectorTest() {
    delete [] array_interleaved_;
  }

  virtual void SetUp() {
    
    for (size_t i = 0; i < kLength; ++i) {
      array_[i] = static_cast<T>(i);
    }
    T* ptr = array_interleaved_;
    
    
    
    for (size_t i = 0; i < kLength; ++i) {
      for (size_t j = 1; j <= num_channels_; ++j) {
        *ptr = j * 100 + i;
        ++ptr;
      }
    }
  }

  enum {
    kLength = 10
  };

  const size_t num_channels_;
  size_t interleaved_length_;
  T array_[kLength];
  T* array_interleaved_;
};



TEST_P(AudioMultiVectorTest, CreateAndDestroy) {
  AudioMultiVector<T> vec1(num_channels_);
  EXPECT_TRUE(vec1.Empty());
  EXPECT_EQ(num_channels_, vec1.Channels());
  EXPECT_EQ(0u, vec1.Size());

  size_t initial_size = 17;
  AudioMultiVector<T> vec2(num_channels_, initial_size);
  EXPECT_FALSE(vec2.Empty());
  EXPECT_EQ(num_channels_, vec2.Channels());
  EXPECT_EQ(initial_size, vec2.Size());
}


TEST_P(AudioMultiVectorTest, SubscriptOperator) {
  AudioMultiVector<T> vec(num_channels_, kLength);
  for (size_t channel = 0; channel < num_channels_; ++channel) {
    for (size_t i = 0; i < kLength; ++i) {
      vec[channel][i] = static_cast<T>(i);
      
      const AudioVector<T>& audio_vec = vec[channel];
      EXPECT_EQ(static_cast<T>(i), audio_vec[i]);
    }
  }
}



TEST_P(AudioMultiVectorTest, PushBackInterleavedAndCopy) {
  AudioMultiVector<T> vec(num_channels_);
  vec.PushBackInterleaved(array_interleaved_, interleaved_length_);
  AudioMultiVector<T> vec_copy(num_channels_);
  vec.CopyFrom(&vec_copy);  
  ASSERT_EQ(num_channels_, vec.Channels());
  ASSERT_EQ(kLength, vec.Size());
  ASSERT_EQ(num_channels_, vec_copy.Channels());
  ASSERT_EQ(kLength, vec_copy.Size());
  for (size_t channel = 0; channel < vec.Channels(); ++channel) {
    for (size_t i = 0; i < kLength; ++i) {
      EXPECT_EQ(static_cast<T>((channel + 1) * 100 + i), vec[channel][i]);
      EXPECT_EQ(vec[channel][i], vec_copy[channel][i]);
    }
  }

  
  vec.Clear();
  EXPECT_TRUE(vec.Empty());

  
  vec.CopyFrom(&vec_copy);
  EXPECT_TRUE(vec_copy.Empty());
}


TEST_P(AudioMultiVectorTest, CopyToNull) {
  AudioMultiVector<T> vec(num_channels_);
  AudioMultiVector<T>* vec_copy = NULL;
  vec.PushBackInterleaved(array_interleaved_, interleaved_length_);
  vec.CopyFrom(vec_copy);
}


TEST_P(AudioMultiVectorTest, PushBackVector) {
  AudioMultiVector<T> vec1(num_channels_, kLength);
  AudioMultiVector<T> vec2(num_channels_, kLength);
  
  
  
  for (size_t channel = 0; channel < num_channels_; ++channel) {
    for (size_t i = 0; i < kLength; ++i) {
      vec1[channel][i] = static_cast<T>(i + 100 * channel);
      vec2[channel][i] = static_cast<T>(i + 100 * channel + kLength);
    }
  }
  
  vec1.PushBack(vec2);
  ASSERT_EQ(2u * kLength, vec1.Size());
  for (size_t channel = 0; channel < num_channels_; ++channel) {
    for (size_t i = 0; i < 2 * kLength; ++i) {
      EXPECT_EQ(static_cast<T>(i + 100 * channel), vec1[channel][i]);
    }
  }
}


TEST_P(AudioMultiVectorTest, PushBackFromIndex) {
  AudioMultiVector<T> vec1(num_channels_);
  vec1.PushBackInterleaved(array_interleaved_, interleaved_length_);
  AudioMultiVector<T> vec2(num_channels_);

  
  
  vec2.PushBackFromIndex(vec1, kLength - 2);
  ASSERT_EQ(2u, vec2.Size());
  for (size_t channel = 0; channel < num_channels_; ++channel) {
    for (size_t i = 0; i < 2; ++i) {
      EXPECT_EQ(array_interleaved_[channel + num_channels_ * (kLength - 2 + i)],
                vec2[channel][i]);
    }
  }
}


TEST_P(AudioMultiVectorTest, Zeros) {
  AudioMultiVector<T> vec(num_channels_);
  vec.PushBackInterleaved(array_interleaved_, interleaved_length_);
  vec.Zeros(2 * kLength);
  ASSERT_EQ(num_channels_, vec.Channels());
  ASSERT_EQ(2u * kLength, vec.Size());
  for (size_t channel = 0; channel < num_channels_; ++channel) {
    for (size_t i = 0; i < 2 * kLength; ++i) {
      EXPECT_EQ(0, vec[channel][i]);
    }
  }
}


TEST_P(AudioMultiVectorTest, ReadInterleaved) {
  AudioMultiVector<T> vec(num_channels_);
  vec.PushBackInterleaved(array_interleaved_, interleaved_length_);
  T* output = new T[interleaved_length_];
  
  size_t read_samples = 5;
  EXPECT_EQ(num_channels_ * read_samples,
            vec.ReadInterleaved(read_samples, output));
  EXPECT_EQ(0, memcmp(array_interleaved_, output, read_samples * sizeof(T)));

  
  EXPECT_EQ(interleaved_length_,
            vec.ReadInterleaved(kLength + 1, output));
  EXPECT_EQ(0, memcmp(array_interleaved_, output, read_samples * sizeof(T)));

  delete [] output;
}


TEST_P(AudioMultiVectorTest, ReadInterleavedToNull) {
  AudioMultiVector<T> vec(num_channels_);
  vec.PushBackInterleaved(array_interleaved_, interleaved_length_);
  T* output = NULL;
  
  size_t read_samples = 5;
  EXPECT_EQ(0u, vec.ReadInterleaved(read_samples, output));
}


TEST_P(AudioMultiVectorTest, PopFront) {
  AudioMultiVector<T> vec(num_channels_);
  vec.PushBackInterleaved(array_interleaved_, interleaved_length_);
  vec.PopFront(1);  
  ASSERT_EQ(kLength - 1u, vec.Size());
  
  
  T* ptr = &array_interleaved_[num_channels_];
  for (size_t i = 0; i < kLength - 1; ++i) {
    for (size_t channel = 0; channel < num_channels_; ++channel) {
      EXPECT_EQ(*ptr, vec[channel][i]);
      ++ptr;
    }
  }
  vec.PopFront(kLength);  
  EXPECT_EQ(0u, vec.Size());
}


TEST_P(AudioMultiVectorTest, PopBack) {
  AudioMultiVector<T> vec(num_channels_);
  vec.PushBackInterleaved(array_interleaved_, interleaved_length_);
  vec.PopBack(1);  
  ASSERT_EQ(kLength - 1u, vec.Size());
  
  
  T* ptr = array_interleaved_;
  for (size_t i = 0; i < kLength - 1; ++i) {
    for (size_t channel = 0; channel < num_channels_; ++channel) {
      EXPECT_EQ(*ptr, vec[channel][i]);
      ++ptr;
    }
  }
  vec.PopBack(kLength);  
  EXPECT_EQ(0u, vec.Size());
}


TEST_P(AudioMultiVectorTest, AssertSize) {
  AudioMultiVector<T> vec(num_channels_, kLength);
  EXPECT_EQ(kLength, vec.Size());
  
  vec.AssertSize(0);
  vec.AssertSize(kLength - 1);
  
  EXPECT_EQ(kLength, vec.Size());
  
  vec.AssertSize(kLength + 1);
  
  EXPECT_EQ(kLength + 1u, vec.Size());
  
  for (size_t channel = 0; channel < vec.Channels(); ++channel) {
    EXPECT_EQ(kLength + 1u, vec[channel].Size());
  }
}


TEST_P(AudioMultiVectorTest, OverwriteAt) {
  AudioMultiVector<T> vec1(num_channels_);
  vec1.PushBackInterleaved(array_interleaved_, interleaved_length_);
  AudioMultiVector<T> vec2(num_channels_);
  vec2.Zeros(3);  
  
  vec1.OverwriteAt(vec2, 3, 5);
  
  ASSERT_EQ(kLength, vec1.Size());  
  T* ptr = array_interleaved_;
  for (size_t i = 0; i < kLength - 1; ++i) {
    for (size_t channel = 0; channel < num_channels_; ++channel) {
      if (i >= 5 && i <= 7) {
        
        EXPECT_EQ(0, vec1[channel][i]);
      } else {
        EXPECT_EQ(*ptr, vec1[channel][i]);
      }
      ++ptr;
    }
  }
}

INSTANTIATE_TEST_CASE_P(TestNumChannels,
                        AudioMultiVectorTest,
                        ::testing::Values(static_cast<size_t>(1),
                                          static_cast<size_t>(2),
                                          static_cast<size_t>(5)));
}  
