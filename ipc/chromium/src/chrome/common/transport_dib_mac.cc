



#include "chrome/common/transport_dib.h"

#include <unistd.h>
#include <sys/stat.h>

#include "base/eintr_wrapper.h"
#include "base/shared_memory.h"

TransportDIB::TransportDIB()
    : size_(0) {
}

TransportDIB::TransportDIB(TransportDIB::Handle dib)
    : shared_memory_(dib, false ),
      size_(0) {
}

TransportDIB::~TransportDIB() {
}


TransportDIB* TransportDIB::Create(size_t size, uint32 sequence_num) {
  TransportDIB* dib = new TransportDIB;
  if (!dib->shared_memory_.Create(L"", false ,
                                  false , size)) {
    delete dib;
    return NULL;
  }

  dib->size_ = size;
  return dib;
}


TransportDIB* TransportDIB::Map(TransportDIB::Handle handle) {
  TransportDIB* dib = new TransportDIB(handle);
  struct stat st;
  fstat(handle.fd, &st);

  if (!dib->shared_memory_.Map(st.st_size)) {
    delete dib;
    HANDLE_EINTR(close(handle.fd));
    return false;
  }

  dib->size_ = st.st_size;

  return dib;
}

void* TransportDIB::memory() const {
  return shared_memory_.memory();
}

TransportDIB::Id TransportDIB::id() const {
  return shared_memory_.id();
}

TransportDIB::Handle TransportDIB::handle() const {
  return shared_memory_.handle();
}
