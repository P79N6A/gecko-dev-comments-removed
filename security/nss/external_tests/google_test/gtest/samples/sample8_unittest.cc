



































#include "prime_tables.h"

#include "gtest/gtest.h"

#if GTEST_HAS_COMBINE








class HybridPrimeTable : public PrimeTable {
 public:
  HybridPrimeTable(bool force_on_the_fly, int max_precalculated)
      : on_the_fly_impl_(new OnTheFlyPrimeTable),
        precalc_impl_(force_on_the_fly ? NULL :
                          new PreCalculatedPrimeTable(max_precalculated)),
        max_precalculated_(max_precalculated) {}
  virtual ~HybridPrimeTable() {
    delete on_the_fly_impl_;
    delete precalc_impl_;
  }

  virtual bool IsPrime(int n) const {
    if (precalc_impl_ != NULL && n < max_precalculated_)
      return precalc_impl_->IsPrime(n);
    else
      return on_the_fly_impl_->IsPrime(n);
  }

  virtual int GetNextPrime(int p) const {
    int next_prime = -1;
    if (precalc_impl_ != NULL && p < max_precalculated_)
      next_prime = precalc_impl_->GetNextPrime(p);

    return next_prime != -1 ? next_prime : on_the_fly_impl_->GetNextPrime(p);
  }

 private:
  OnTheFlyPrimeTable* on_the_fly_impl_;
  PreCalculatedPrimeTable* precalc_impl_;
  int max_precalculated_;
};

using ::testing::TestWithParam;
using ::testing::Bool;
using ::testing::Values;
using ::testing::Combine;






class PrimeTableTest : public TestWithParam< ::testing::tuple<bool, int> > {
 protected:
  virtual void SetUp() {
    
    
    
    
    
    
    
    
    bool force_on_the_fly = ::testing::get<0>(GetParam());
    int max_precalculated = ::testing::get<1>(GetParam());
    table_ = new HybridPrimeTable(force_on_the_fly, max_precalculated);
  }
  virtual void TearDown() {
    delete table_;
    table_ = NULL;
  }
  HybridPrimeTable* table_;
};

TEST_P(PrimeTableTest, ReturnsFalseForNonPrimes) {
  
  
  
  
  

  EXPECT_FALSE(table_->IsPrime(-5));
  EXPECT_FALSE(table_->IsPrime(0));
  EXPECT_FALSE(table_->IsPrime(1));
  EXPECT_FALSE(table_->IsPrime(4));
  EXPECT_FALSE(table_->IsPrime(6));
  EXPECT_FALSE(table_->IsPrime(100));
}

TEST_P(PrimeTableTest, ReturnsTrueForPrimes) {
  EXPECT_TRUE(table_->IsPrime(2));
  EXPECT_TRUE(table_->IsPrime(3));
  EXPECT_TRUE(table_->IsPrime(5));
  EXPECT_TRUE(table_->IsPrime(7));
  EXPECT_TRUE(table_->IsPrime(11));
  EXPECT_TRUE(table_->IsPrime(131));
}

TEST_P(PrimeTableTest, CanGetNextPrime) {
  EXPECT_EQ(2, table_->GetNextPrime(0));
  EXPECT_EQ(3, table_->GetNextPrime(2));
  EXPECT_EQ(5, table_->GetNextPrime(3));
  EXPECT_EQ(7, table_->GetNextPrime(5));
  EXPECT_EQ(11, table_->GetNextPrime(7));
  EXPECT_EQ(131, table_->GetNextPrime(128));
}












INSTANTIATE_TEST_CASE_P(MeaningfulTestParameters,
                        PrimeTableTest,
                        Combine(Bool(), Values(1, 10)));

#else







TEST(DummyTest, CombineIsNotSupportedOnThisPlatform) {}

#endif  
