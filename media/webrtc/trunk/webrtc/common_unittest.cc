








#include "common.h"  

#include "gtest/gtest.h"

namespace webrtc {
namespace {

struct MyExperiment {
  enum { kDefaultFactor = 1 };
  enum { kDefaultOffset = 2 };

  MyExperiment()
    : factor(kDefaultFactor), offset(kDefaultOffset) {}

  MyExperiment(int factor, int offset)
    : factor(factor), offset(offset) {}

  int factor;
  int offset;
};

TEST(Config, ReturnsDefaultInstanceIfNotConfigured) {
  Config config;
  const MyExperiment& my_exp = config.Get<MyExperiment>();
  EXPECT_EQ(MyExperiment::kDefaultFactor, my_exp.factor);
  EXPECT_EQ(MyExperiment::kDefaultOffset, my_exp.offset);
}

TEST(Config, ReturnOptionWhenSet) {
  Config config;
  config.Set<MyExperiment>(new MyExperiment(5, 1));
  const MyExperiment& my_exp = config.Get<MyExperiment>();
  EXPECT_EQ(5, my_exp.factor);
  EXPECT_EQ(1, my_exp.offset);
}

TEST(Config, SetNullSetsTheOptionBackToDefault) {
  Config config;
  config.Set<MyExperiment>(new MyExperiment(5, 1));
  config.Set<MyExperiment>(NULL);
  const MyExperiment& my_exp = config.Get<MyExperiment>();
  EXPECT_EQ(MyExperiment::kDefaultFactor, my_exp.factor);
  EXPECT_EQ(MyExperiment::kDefaultOffset, my_exp.offset);
}

struct Algo1_CostFunction {
  Algo1_CostFunction() {}

  virtual int cost(int x) const {
    return x;
  }

  virtual ~Algo1_CostFunction() {}
};

struct SqrCost : Algo1_CostFunction {
  virtual int cost(int x) const {
    return x*x;
  }
};

TEST(Config, SupportsPolimorphism) {
  Config config;
  config.Set<Algo1_CostFunction>(new SqrCost());
  EXPECT_EQ(25, config.Get<Algo1_CostFunction>().cost(5));
}
}  
}  
