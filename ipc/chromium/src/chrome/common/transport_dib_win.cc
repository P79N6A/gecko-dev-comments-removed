



#include <limits>
#include <windows.h>

#include "base/logging.h"
#include "base/sys_info.h"
#include "chrome/common/transport_dib.h"

TransportDIB::TransportDIB() {
}

TransportDIB::~TransportDIB() {
}

TransportDIB::TransportDIB(HANDLE handle)
    : shared_memory_(handle, false ) {
}


TransportDIB* TransportDIB::Create(size_t size, uint32 sequence_num) {
  size_t allocation_granularity = base::SysInfo::VMAllocationGranularity();
  size = size / allocation_granularity + 1;
  size = size * allocation_granularity;

  TransportDIB* dib = new TransportDIB;

#ifdef CHROMIUM_MOZILLA_BUILD
  if (!dib->shared_memory_.Create("", false ,
#else
  if (!dib->shared_memory_.Create(L"", false ,
#endif
                                  true , size)) {
    delete dib;
    return NULL;
  }

  dib->size_ = size;
  dib->sequence_num_ = sequence_num;

  return dib;
}


TransportDIB* TransportDIB::Map(TransportDIB::Handle handle) {
  TransportDIB* dib = new TransportDIB(handle);
  if (!dib->shared_memory_.Map(0 )) {
    LOG(ERROR) << "Failed to map transport DIB"
               << " handle:" << handle
               << " error:" << GetLastError();
    delete dib;
    return NULL;
  }

  
  
  
  
  dib->size_ = std::numeric_limits<size_t>::max();

  return dib;
}

void* TransportDIB::memory() const {
  return shared_memory_.memory();
}

TransportDIB::Handle TransportDIB::handle() const {
  return shared_memory_.handle();
}

TransportDIB::Id TransportDIB::id() const {
  return Id(shared_memory_.handle(), sequence_num_);
}
