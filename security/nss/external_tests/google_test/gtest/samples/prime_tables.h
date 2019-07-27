



































#ifndef GTEST_SAMPLES_PRIME_TABLES_H_
#define GTEST_SAMPLES_PRIME_TABLES_H_

#include <algorithm>


class PrimeTable {
 public:
  virtual ~PrimeTable() {}

  
  virtual bool IsPrime(int n) const = 0;

  
  
  virtual int GetNextPrime(int p) const = 0;
};


class OnTheFlyPrimeTable : public PrimeTable {
 public:
  virtual bool IsPrime(int n) const {
    if (n <= 1) return false;

    for (int i = 2; i*i <= n; i++) {
      
      if ((n % i) == 0) return false;
    }

    return true;
  }

  virtual int GetNextPrime(int p) const {
    for (int n = p + 1; n > 0; n++) {
      if (IsPrime(n)) return n;
    }

    return -1;
  }
};



class PreCalculatedPrimeTable : public PrimeTable {
 public:
  
  explicit PreCalculatedPrimeTable(int max)
      : is_prime_size_(max + 1), is_prime_(new bool[max + 1]) {
    CalculatePrimesUpTo(max);
  }
  virtual ~PreCalculatedPrimeTable() { delete[] is_prime_; }

  virtual bool IsPrime(int n) const {
    return 0 <= n && n < is_prime_size_ && is_prime_[n];
  }

  virtual int GetNextPrime(int p) const {
    for (int n = p + 1; n < is_prime_size_; n++) {
      if (is_prime_[n]) return n;
    }

    return -1;
  }

 private:
  void CalculatePrimesUpTo(int max) {
    ::std::fill(is_prime_, is_prime_ + is_prime_size_, true);
    is_prime_[0] = is_prime_[1] = false;

    for (int i = 2; i <= max; i++) {
      if (!is_prime_[i]) continue;

      
      for (int j = 2*i; j <= max; j += i) {
        is_prime_[j] = false;
      }
    }
  }

  const int is_prime_size_;
  bool* const is_prime_;

  
  void operator=(const PreCalculatedPrimeTable& rhs);
};

#endif  
