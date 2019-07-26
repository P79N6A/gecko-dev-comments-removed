

























































#ifndef GMOCK_INCLUDE_GMOCK_GMOCK_GENERATED_NICE_STRICT_H_
#define GMOCK_INCLUDE_GMOCK_GMOCK_GENERATED_NICE_STRICT_H_

#include "gmock/gmock-spec-builders.h"
#include "gmock/internal/gmock-port.h"

namespace testing {

template <class MockClass>
class NiceMock : public MockClass {
 public:
  
  
  NiceMock() {
    ::testing::Mock::AllowUninterestingCalls(
        internal::ImplicitCast_<MockClass*>(this));
  }

  
  
  template <typename A1>
  explicit NiceMock(const A1& a1) : MockClass(a1) {
    ::testing::Mock::AllowUninterestingCalls(
        internal::ImplicitCast_<MockClass*>(this));
  }
  template <typename A1, typename A2>
  NiceMock(const A1& a1, const A2& a2) : MockClass(a1, a2) {
    ::testing::Mock::AllowUninterestingCalls(
        internal::ImplicitCast_<MockClass*>(this));
  }

  template <typename A1, typename A2, typename A3>
  NiceMock(const A1& a1, const A2& a2, const A3& a3) : MockClass(a1, a2, a3) {
    ::testing::Mock::AllowUninterestingCalls(
        internal::ImplicitCast_<MockClass*>(this));
  }

  template <typename A1, typename A2, typename A3, typename A4>
  NiceMock(const A1& a1, const A2& a2, const A3& a3,
      const A4& a4) : MockClass(a1, a2, a3, a4) {
    ::testing::Mock::AllowUninterestingCalls(
        internal::ImplicitCast_<MockClass*>(this));
  }

  template <typename A1, typename A2, typename A3, typename A4, typename A5>
  NiceMock(const A1& a1, const A2& a2, const A3& a3, const A4& a4,
      const A5& a5) : MockClass(a1, a2, a3, a4, a5) {
    ::testing::Mock::AllowUninterestingCalls(
        internal::ImplicitCast_<MockClass*>(this));
  }

  template <typename A1, typename A2, typename A3, typename A4, typename A5,
      typename A6>
  NiceMock(const A1& a1, const A2& a2, const A3& a3, const A4& a4,
      const A5& a5, const A6& a6) : MockClass(a1, a2, a3, a4, a5, a6) {
    ::testing::Mock::AllowUninterestingCalls(
        internal::ImplicitCast_<MockClass*>(this));
  }

  template <typename A1, typename A2, typename A3, typename A4, typename A5,
      typename A6, typename A7>
  NiceMock(const A1& a1, const A2& a2, const A3& a3, const A4& a4,
      const A5& a5, const A6& a6, const A7& a7) : MockClass(a1, a2, a3, a4, a5,
      a6, a7) {
    ::testing::Mock::AllowUninterestingCalls(
        internal::ImplicitCast_<MockClass*>(this));
  }

  template <typename A1, typename A2, typename A3, typename A4, typename A5,
      typename A6, typename A7, typename A8>
  NiceMock(const A1& a1, const A2& a2, const A3& a3, const A4& a4,
      const A5& a5, const A6& a6, const A7& a7, const A8& a8) : MockClass(a1,
      a2, a3, a4, a5, a6, a7, a8) {
    ::testing::Mock::AllowUninterestingCalls(
        internal::ImplicitCast_<MockClass*>(this));
  }

  template <typename A1, typename A2, typename A3, typename A4, typename A5,
      typename A6, typename A7, typename A8, typename A9>
  NiceMock(const A1& a1, const A2& a2, const A3& a3, const A4& a4,
      const A5& a5, const A6& a6, const A7& a7, const A8& a8,
      const A9& a9) : MockClass(a1, a2, a3, a4, a5, a6, a7, a8, a9) {
    ::testing::Mock::AllowUninterestingCalls(
        internal::ImplicitCast_<MockClass*>(this));
  }

  template <typename A1, typename A2, typename A3, typename A4, typename A5,
      typename A6, typename A7, typename A8, typename A9, typename A10>
  NiceMock(const A1& a1, const A2& a2, const A3& a3, const A4& a4,
      const A5& a5, const A6& a6, const A7& a7, const A8& a8, const A9& a9,
      const A10& a10) : MockClass(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) {
    ::testing::Mock::AllowUninterestingCalls(
        internal::ImplicitCast_<MockClass*>(this));
  }

  virtual ~NiceMock() {
    ::testing::Mock::UnregisterCallReaction(
        internal::ImplicitCast_<MockClass*>(this));
  }

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(NiceMock);
};

template <class MockClass>
class StrictMock : public MockClass {
 public:
  
  
  StrictMock() {
    ::testing::Mock::FailUninterestingCalls(
        internal::ImplicitCast_<MockClass*>(this));
  }

  template <typename A1>
  explicit StrictMock(const A1& a1) : MockClass(a1) {
    ::testing::Mock::FailUninterestingCalls(
        internal::ImplicitCast_<MockClass*>(this));
  }
  template <typename A1, typename A2>
  StrictMock(const A1& a1, const A2& a2) : MockClass(a1, a2) {
    ::testing::Mock::FailUninterestingCalls(
        internal::ImplicitCast_<MockClass*>(this));
  }

  template <typename A1, typename A2, typename A3>
  StrictMock(const A1& a1, const A2& a2, const A3& a3) : MockClass(a1, a2, a3) {
    ::testing::Mock::FailUninterestingCalls(
        internal::ImplicitCast_<MockClass*>(this));
  }

  template <typename A1, typename A2, typename A3, typename A4>
  StrictMock(const A1& a1, const A2& a2, const A3& a3,
      const A4& a4) : MockClass(a1, a2, a3, a4) {
    ::testing::Mock::FailUninterestingCalls(
        internal::ImplicitCast_<MockClass*>(this));
  }

  template <typename A1, typename A2, typename A3, typename A4, typename A5>
  StrictMock(const A1& a1, const A2& a2, const A3& a3, const A4& a4,
      const A5& a5) : MockClass(a1, a2, a3, a4, a5) {
    ::testing::Mock::FailUninterestingCalls(
        internal::ImplicitCast_<MockClass*>(this));
  }

  template <typename A1, typename A2, typename A3, typename A4, typename A5,
      typename A6>
  StrictMock(const A1& a1, const A2& a2, const A3& a3, const A4& a4,
      const A5& a5, const A6& a6) : MockClass(a1, a2, a3, a4, a5, a6) {
    ::testing::Mock::FailUninterestingCalls(
        internal::ImplicitCast_<MockClass*>(this));
  }

  template <typename A1, typename A2, typename A3, typename A4, typename A5,
      typename A6, typename A7>
  StrictMock(const A1& a1, const A2& a2, const A3& a3, const A4& a4,
      const A5& a5, const A6& a6, const A7& a7) : MockClass(a1, a2, a3, a4, a5,
      a6, a7) {
    ::testing::Mock::FailUninterestingCalls(
        internal::ImplicitCast_<MockClass*>(this));
  }

  template <typename A1, typename A2, typename A3, typename A4, typename A5,
      typename A6, typename A7, typename A8>
  StrictMock(const A1& a1, const A2& a2, const A3& a3, const A4& a4,
      const A5& a5, const A6& a6, const A7& a7, const A8& a8) : MockClass(a1,
      a2, a3, a4, a5, a6, a7, a8) {
    ::testing::Mock::FailUninterestingCalls(
        internal::ImplicitCast_<MockClass*>(this));
  }

  template <typename A1, typename A2, typename A3, typename A4, typename A5,
      typename A6, typename A7, typename A8, typename A9>
  StrictMock(const A1& a1, const A2& a2, const A3& a3, const A4& a4,
      const A5& a5, const A6& a6, const A7& a7, const A8& a8,
      const A9& a9) : MockClass(a1, a2, a3, a4, a5, a6, a7, a8, a9) {
    ::testing::Mock::FailUninterestingCalls(
        internal::ImplicitCast_<MockClass*>(this));
  }

  template <typename A1, typename A2, typename A3, typename A4, typename A5,
      typename A6, typename A7, typename A8, typename A9, typename A10>
  StrictMock(const A1& a1, const A2& a2, const A3& a3, const A4& a4,
      const A5& a5, const A6& a6, const A7& a7, const A8& a8, const A9& a9,
      const A10& a10) : MockClass(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) {
    ::testing::Mock::FailUninterestingCalls(
        internal::ImplicitCast_<MockClass*>(this));
  }

  virtual ~StrictMock() {
    ::testing::Mock::UnregisterCallReaction(
        internal::ImplicitCast_<MockClass*>(this));
  }

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(StrictMock);
};







template <typename MockClass>
class NiceMock<NiceMock<MockClass> >;
template <typename MockClass>
class NiceMock<StrictMock<MockClass> >;
template <typename MockClass>
class StrictMock<NiceMock<MockClass> >;
template <typename MockClass>
class StrictMock<StrictMock<MockClass> >;

}  

#endif  
