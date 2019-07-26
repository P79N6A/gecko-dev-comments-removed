





#include "mozilla/Assertions.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/NullPtr.h"

#include "LulRWLock.h"


namespace lul {





#if defined(LUL_OS_linux)

LulRWLock::LulRWLock() {
  mozilla::DebugOnly<int> r = pthread_rwlock_init(&mLock, nullptr);
  MOZ_ASSERT(!r);
}

LulRWLock::~LulRWLock() {
  mozilla::DebugOnly<int>r = pthread_rwlock_destroy(&mLock);
  MOZ_ASSERT(!r);
}

void
LulRWLock::WrLock() {
  mozilla::DebugOnly<int>r = pthread_rwlock_wrlock(&mLock);
  MOZ_ASSERT(!r);
}

void
LulRWLock::RdLock() {
  mozilla::DebugOnly<int>r = pthread_rwlock_rdlock(&mLock);
  MOZ_ASSERT(!r);
}

void
LulRWLock::Unlock() {
  mozilla::DebugOnly<int>r = pthread_rwlock_unlock(&mLock);
  MOZ_ASSERT(!r);
}









#elif defined(LUL_OS_android)

LulRWLock::LulRWLock() {
  mozilla::DebugOnly<int> r = pthread_mutex_init(&mLock, nullptr);
  MOZ_ASSERT(!r);
}

LulRWLock::~LulRWLock() {
  mozilla::DebugOnly<int>r = pthread_mutex_destroy(&mLock);
  MOZ_ASSERT(!r);
}

void
LulRWLock::WrLock() {
  mozilla::DebugOnly<int>r = pthread_mutex_lock(&mLock);
  MOZ_ASSERT(!r);
}

void
LulRWLock::RdLock() {
  mozilla::DebugOnly<int>r = pthread_mutex_lock(&mLock);
  MOZ_ASSERT(!r);
}

void
LulRWLock::Unlock() {
  mozilla::DebugOnly<int>r = pthread_mutex_unlock(&mLock);
  MOZ_ASSERT(!r);
}


#else
# error "Unsupported OS"
#endif

} 
