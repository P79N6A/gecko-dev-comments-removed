
































#ifndef GTEST_SAMPLES_SAMPLE4_H_
#define GTEST_SAMPLES_SAMPLE4_H_


class Counter {
 private:
  int counter_;

 public:
  
  Counter() : counter_(0) {}

  
  int Increment();

  
  void Print() const;
};

#endif  
