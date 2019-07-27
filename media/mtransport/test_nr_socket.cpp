

















































































extern "C" {
#include "stun_msg.h" 
#include "nr_api.h"
#include "async_wait.h"
#include "nr_socket.h"
#include "nr_socket_local.h"
#include "stun_hint.h"
#include "transport_addr.h"
}

#include "mozilla/RefPtr.h"
#include "test_nr_socket.h"
#include "runnable_utils.h"

namespace mozilla {

static int test_nat_socket_create(void *obj,
                                  nr_transport_addr *addr,
                                  nr_socket **sockp) {
  RefPtr<NrSocketBase> sock = new TestNrSocket(static_cast<TestNat*>(obj));

  int r, _status;

  r = sock->create(addr);
  if (r)
    ABORT(r);

  r = nr_socket_create_int(static_cast<void *>(sock),
                           sock->vtbl(), sockp);
  if (r)
    ABORT(r);

  _status = 0;

  {
    
    
    NrSocketBase *dummy = sock.forget().take();
    (void)dummy;
  }

abort:
  return _status;
}

static int test_nat_socket_factory_destroy(void **obj) {
  TestNat *nat = static_cast<TestNat*>(*obj);
  *obj = nullptr;
  nat->Release();
  return 0;
}

static nr_socket_factory_vtbl test_nat_socket_factory_vtbl = {
  test_nat_socket_create,
  test_nat_socket_factory_destroy
};

bool TestNat::has_port_mappings() const {
  for (TestNrSocket *sock : sockets_) {
    if (sock->has_port_mappings()) {
      return true;
    }
  }
  return false;
}

bool TestNat::is_my_external_tuple(const nr_transport_addr &addr) const {
  for (TestNrSocket *sock : sockets_) {
    if (sock->is_my_external_tuple(addr)) {
      return true;
    }
  }

  return false;
}

bool TestNat::is_an_internal_tuple(const nr_transport_addr &addr) const {
  for (TestNrSocket *sock : sockets_) {
    nr_transport_addr addr_behind_nat;
    if (sock->getaddr(&addr_behind_nat)) {
      MOZ_CRASH("TestNrSocket::getaddr failed!");
    }

    
    if (!nr_transport_addr_cmp(const_cast<nr_transport_addr*>(&addr),
                               &addr_behind_nat,
                               NR_TRANSPORT_ADDR_CMP_MODE_ALL)) {
      return true;
    }
  }
  return false;
}

int TestNat::create_socket_factory(nr_socket_factory **factorypp) {
  int r = nr_socket_factory_create_int(this,
                                       &test_nat_socket_factory_vtbl,
                                       factorypp);
  if (!r) {
    AddRef();
  }
  return r;
}

TestNrSocket::TestNrSocket(TestNat *nat)
  : nat_(nat) {
  nat_->insert_socket(this);
}

TestNrSocket::~TestNrSocket() {
  nat_->erase_socket(this);
}

nsRefPtr<NrSocket> TestNrSocket::create_external_socket(
    const nr_transport_addr &dest_addr) const {
  MOZ_ASSERT(nat_->enabled_);
  MOZ_ASSERT(!nat_->is_an_internal_tuple(dest_addr));

  int r;
  nr_transport_addr nat_external_addr;

  
  
  if ((r = nr_transport_addr_copy(&nat_external_addr,
                                  const_cast<nr_transport_addr*>(&my_addr_)))) {
    r_log(LOG_GENERIC,LOG_CRIT, "%s: Failure in nr_transport_addr_copy: %d",
                                __FUNCTION__, r);
    return nullptr;
  }

  if ((r = nr_transport_addr_set_port(&nat_external_addr, 0))) {
    r_log(LOG_GENERIC,LOG_CRIT, "%s: Failure in nr_transport_addr_set_port: %d",
                                __FUNCTION__, r);
    return nullptr;
  }

  nsRefPtr<NrSocket> external_socket = new NrSocket;

  if ((r = external_socket->create(&nat_external_addr))) {
    r_log(LOG_GENERIC,LOG_CRIT, "%s: Failure in NrSocket::create: %d",
                                __FUNCTION__, r);
    return nullptr;
  }

  return external_socket;
}

int TestNrSocket::sendto(const void *msg, size_t len,
                         int flags, nr_transport_addr *to) {
  MOZ_ASSERT(my_addr_.protocol != IPPROTO_TCP);
  ASSERT_ON_THREAD(ststhread_);

  if (!nat_->enabled_ || nat_->is_an_internal_tuple(*to)) {
    return NrSocket::sendto(msg, len, flags, to);
  }

  destroy_stale_port_mappings();

  if (to->protocol == IPPROTO_UDP && nat_->block_udp_) {
    
    return 0;
  }

  
  PortMapping *port_mapping = get_port_mapping(*to,
                                               std::max(nat_->filtering_type_,
                                                        nat_->mapping_type_));

  if (!port_mapping) {
    
    PortMapping *similar_port_mapping =
      get_port_mapping(*to, nat_->mapping_type_);
    nsRefPtr<NrSocket> external_socket;

    if (similar_port_mapping) {
      external_socket = similar_port_mapping->external_socket_;
    } else {
      external_socket = create_external_socket(*to);
      if (!external_socket) {
        MOZ_ASSERT(false);
        return R_INTERNAL;
      }
    }

    port_mapping = create_port_mapping(*to, external_socket);
    port_mappings_.push_back(port_mapping);

    if (poll_flags() & PR_POLL_READ) {
      
      
      port_mapping->async_wait(NR_ASYNC_WAIT_READ,
                              port_mapping_readable_callback,
                              this,
                              (char*)__FUNCTION__,
                              __LINE__);
    }
  }

  
  
  return port_mapping->sendto(msg, len, *to);
}

int TestNrSocket::recvfrom(void *buf, size_t maxlen,
                           size_t *len, int flags,
                           nr_transport_addr *from) {
  MOZ_ASSERT(my_addr_.protocol != IPPROTO_TCP);
  ASSERT_ON_THREAD(ststhread_);

  int r;
  bool ingress_allowed = false;

  if (readable_socket_) {
    
    r = readable_socket_->recvfrom(buf, maxlen, len, 0, from);
    readable_socket_ = nullptr;
    if (!r) {
      PortMapping *port_mapping_used;
      ingress_allowed = allow_ingress(*from, &port_mapping_used);
      if (ingress_allowed && nat_->refresh_on_ingress_ && port_mapping_used) {
        port_mapping_used->last_used_ = PR_IntervalNow();
      }
    }
  } else {
    
    
    
    r = NrSocket::recvfrom(buf, maxlen, len, flags, from);
    if (!r) {
      
      
      ingress_allowed = (!nat_->enabled_ ||
                         nat_->is_an_internal_tuple(*from));
      if (!ingress_allowed) {
        r_log(LOG_GENERIC, LOG_INFO, "TestNrSocket %s denying ingress from %s: "
                                     "Not behind the same NAT",
                                     my_addr_.as_string,
                                     from->as_string);
      }
    }
  }

  
  
  
  
  
  
  if (!ingress_allowed) {
    *len = 0;
    r = R_WOULDBLOCK;
  }

  return r;
}

bool TestNrSocket::allow_ingress(const nr_transport_addr &from,
                                 PortMapping **port_mapping_used) const {
  *port_mapping_used = nullptr;
  if (!nat_->enabled_)
    return true;

  if (nat_->is_an_internal_tuple(from))
    return true;

  *port_mapping_used = get_port_mapping(from, nat_->filtering_type_);
  if (!(*port_mapping_used)) {
    r_log(LOG_GENERIC, LOG_INFO, "TestNrSocket %s denying ingress from %s: "
                                 "Filtered",
                                 my_addr_.as_string,
                                 from.as_string);
    return false;
  }

  if (is_port_mapping_stale(**port_mapping_used)) {
    r_log(LOG_GENERIC, LOG_INFO, "TestNrSocket %s denying ingress from %s: "
                                 "Stale port mapping",
                                 my_addr_.as_string,
                                 from.as_string);
    return false;
  }

  if (!nat_->allow_hairpinning_ && nat_->is_my_external_tuple(from)) {
    r_log(LOG_GENERIC, LOG_INFO, "TestNrSocket %s denying ingress from %s: "
                                 "Hairpinning disallowed",
                                 my_addr_.as_string,
                                 from.as_string);
    return false;
  }

  return true;
}

int TestNrSocket::connect(nr_transport_addr *addr) {
  ASSERT_ON_THREAD(ststhread_);

  if (connect_invoked_ || !port_mappings_.empty()) {
    MOZ_CRASH("TestNrSocket::connect() called more than once!");
    return R_INTERNAL;
  }

  if (!nat_->enabled_ || nat_->is_an_internal_tuple(*addr)) {
    
    return NrSocket::connect(addr);
  }

  nsRefPtr<NrSocket> external_socket(create_external_socket(*addr));
  if (!external_socket) {
    return R_INTERNAL;
  }

  PortMapping *port_mapping = create_port_mapping(*addr, external_socket);
  port_mappings_.push_back(port_mapping);
  port_mapping->external_socket_->connect(addr);
  port_mapping->last_used_ = PR_IntervalNow();

  if (poll_flags() & PR_POLL_READ) {
    port_mapping->async_wait(NR_ASYNC_WAIT_READ,
                             port_mapping_tcp_passthrough_callback,
                             this,
                             (char*)__FUNCTION__,
                             __LINE__);
  }

  return 0;
}

int TestNrSocket::write(const void *msg, size_t len, size_t *written) {
  ASSERT_ON_THREAD(ststhread_);

  if (port_mappings_.empty()) {
    
    r_log(LOG_GENERIC, LOG_INFO, "TestNrSocket %s writing",
          my_addr().as_string);

    return NrSocket::write(msg, len, written);
  } else {
    
    MOZ_ASSERT(port_mappings_.size() == 1);
    r_log(LOG_GENERIC, LOG_INFO,
          "PortMapping %s -> %s writing",
          port_mappings_.front()->external_socket_->my_addr().as_string,
          port_mappings_.front()->remote_address_.as_string);

    return port_mappings_.front()->external_socket_->write(msg, len, written);
  }
}

int TestNrSocket::read(void *buf, size_t maxlen, size_t *len) {
  ASSERT_ON_THREAD(ststhread_);

  if (port_mappings_.empty()) {
    return NrSocket::read(buf, maxlen, len);
  } else {
    MOZ_ASSERT(port_mappings_.size() == 1);
    return port_mappings_.front()->external_socket_->read(buf, maxlen, len);
  }
}

int TestNrSocket::async_wait(int how, NR_async_cb cb, void *cb_arg,
                             char *function, int line) {
  ASSERT_ON_THREAD(ststhread_);

  
  int r = NrSocket::async_wait(how, cb, cb_arg, function, line);

  if (r) {
    return r;
  }

  r_log(LOG_GENERIC, LOG_DEBUG, "TestNrSocket %s waiting for %s",
                                my_addr_.as_string,
                                how == NR_ASYNC_WAIT_READ ? "read" : "write");

  if (is_tcp_connection_behind_nat()) {
    
    return 0;
  }

  if (my_addr_.protocol == IPPROTO_TCP) {
    
    
    MOZ_ASSERT(port_mappings_.size() == 1);

    return port_mappings_.front()->async_wait(
        how,
        port_mapping_tcp_passthrough_callback,
        this,
        function,
        line);
  } else if (how == NR_ASYNC_WAIT_READ) {
    
    for (PortMapping *port_mapping : port_mappings_) {
      
      r = port_mapping->async_wait(how,
                                   port_mapping_readable_callback,
                                   this,
                                   function,
                                   line);
      if (r) {
        return r;
      }
    }
  }

  return 0;
}

void TestNrSocket::cancel_port_mapping_async_wait(int how) {
  for (PortMapping *port_mapping : port_mappings_) {
    port_mapping->cancel(how);
  }
}

int TestNrSocket::cancel(int how) {
  ASSERT_ON_THREAD(ststhread_);

  r_log(LOG_GENERIC, LOG_DEBUG, "TestNrSocket %s stop waiting for %s",
        my_addr_.as_string,
        how == NR_ASYNC_WAIT_READ ? "read" : "write");

  
  if (how == NR_ASYNC_WAIT_READ || my_addr_.protocol == IPPROTO_TCP) {
    cancel_port_mapping_async_wait(how);
  }

  return NrSocket::cancel(how);
}

bool TestNrSocket::has_port_mappings() const {
  return !port_mappings_.empty();
}

bool TestNrSocket::is_my_external_tuple(const nr_transport_addr &addr) const {
  for (PortMapping *port_mapping : port_mappings_) {
    nr_transport_addr port_mapping_addr;
    if (port_mapping->external_socket_->getaddr(&port_mapping_addr)) {
      MOZ_CRASH("NrSocket::getaddr failed!");
    }

    
    if (!nr_transport_addr_cmp(const_cast<nr_transport_addr*>(&addr),
                               &port_mapping_addr,
                               NR_TRANSPORT_ADDR_CMP_MODE_ALL)) {
      return true;
    }
  }
  return false;
}

bool TestNrSocket::is_port_mapping_stale(
    const PortMapping &port_mapping) const {
  PRIntervalTime now = PR_IntervalNow();
  PRIntervalTime elapsed_ticks = now - port_mapping.last_used_;
  uint32_t idle_duration = PR_IntervalToMilliseconds(elapsed_ticks);
  return idle_duration > nat_->mapping_timeout_;
}

void TestNrSocket::destroy_stale_port_mappings() {
  for (auto i = port_mappings_.begin(); i != port_mappings_.end();) {
    auto temp = i;
    ++i;
    if (is_port_mapping_stale(**temp)) {
      r_log(LOG_GENERIC, LOG_INFO,
            "TestNrSocket %s destroying port mapping %s -> %s",
            my_addr_.as_string,
            (*temp)->external_socket_->my_addr().as_string,
            (*temp)->remote_address_.as_string);

      port_mappings_.erase(temp);
    }
  }
}

void TestNrSocket::port_mapping_readable_callback(void *ext_sock_v,
                                                  int how,
                                                  void *test_sock_v) {
  TestNrSocket *test_socket = static_cast<TestNrSocket*>(test_sock_v);
  NrSocket *external_socket = static_cast<NrSocket*>(ext_sock_v);

  test_socket->on_port_mapping_readable(external_socket);
}

void TestNrSocket::on_port_mapping_readable(NrSocket *external_socket) {
  if (!readable_socket_) {
    readable_socket_ = external_socket;
  }

  
  
  MOZ_ASSERT(poll_flags() & PR_POLL_READ);

  fire_readable_callback();
}

void TestNrSocket::fire_readable_callback() {
  MOZ_ASSERT(poll_flags() & PR_POLL_READ);
  
  
  cancel_port_mapping_async_wait(NR_ASYNC_WAIT_READ);
  r_log(LOG_GENERIC, LOG_DEBUG, "TestNrSocket %s ready for read",
        my_addr_.as_string);
  fire_callback(NR_ASYNC_WAIT_READ);
}

void TestNrSocket::port_mapping_writeable_callback(void *ext_sock_v,
                                                   int how,
                                                   void *test_sock_v) {
  TestNrSocket *test_socket = static_cast<TestNrSocket*>(test_sock_v);
  NrSocket *external_socket = static_cast<NrSocket*>(ext_sock_v);

  test_socket->write_to_port_mapping(external_socket);
}

void TestNrSocket::write_to_port_mapping(NrSocket *external_socket) {
  MOZ_ASSERT(my_addr_.protocol != IPPROTO_TCP);

  int r = 0;
  for (PortMapping *port_mapping : port_mappings_) {
    if (port_mapping->external_socket_ == external_socket) {
      
      r = port_mapping->send_from_queue();
      if (r) {
        break;
      }
    }
  }

  if (r == R_WOULDBLOCK) {
    
    NR_ASYNC_WAIT(external_socket,
                  NR_ASYNC_WAIT_WRITE,
                  &TestNrSocket::port_mapping_writeable_callback,
                  this);
  }
}

void TestNrSocket::port_mapping_tcp_passthrough_callback(void *ext_sock_v,
                                                         int how,
                                                         void *test_sock_v) {
  TestNrSocket *test_socket = static_cast<TestNrSocket*>(test_sock_v);
  r_log(LOG_GENERIC, LOG_INFO,
        "TestNrSocket %s firing %s callback",
        test_socket->my_addr().as_string,
        how == NR_ASYNC_WAIT_READ ? "readable" : "writeable");


  test_socket->fire_callback(how);
}

bool TestNrSocket::is_tcp_connection_behind_nat() const {
  return my_addr_.protocol == IPPROTO_TCP && port_mappings_.empty();
}

TestNrSocket::PortMapping* TestNrSocket::get_port_mapping(
    const nr_transport_addr &remote_address,
    TestNat::NatBehavior filter) const {
  int compare_flags;
  switch (filter) {
    case TestNat::ENDPOINT_INDEPENDENT:
      compare_flags = NR_TRANSPORT_ADDR_CMP_MODE_PROTOCOL;
      break;
    case TestNat::ADDRESS_DEPENDENT:
      compare_flags = NR_TRANSPORT_ADDR_CMP_MODE_ADDR;
      break;
    case TestNat::PORT_DEPENDENT:
      compare_flags = NR_TRANSPORT_ADDR_CMP_MODE_ALL;
      break;
  }

  for (PortMapping *port_mapping : port_mappings_) {
    
    if (!nr_transport_addr_cmp(const_cast<nr_transport_addr*>(&remote_address),
                               &port_mapping->remote_address_,
                               compare_flags))
      return port_mapping;
  }
  return nullptr;
}

TestNrSocket::PortMapping* TestNrSocket::create_port_mapping(
    const nr_transport_addr &remote_address,
    const nsRefPtr<NrSocket> &external_socket) const {
  r_log(LOG_GENERIC, LOG_INFO, "TestNrSocket %s creating port mapping %s -> %s",
        my_addr_.as_string,
        external_socket->my_addr().as_string,
        remote_address.as_string);

  return new PortMapping(remote_address, external_socket);
}

TestNrSocket::PortMapping::PortMapping(
    const nr_transport_addr &remote_address,
    const nsRefPtr<NrSocket> &external_socket) :
  external_socket_(external_socket) {
  
  nr_transport_addr_copy(&remote_address_,
                         const_cast<nr_transport_addr*>(&remote_address));
}

int TestNrSocket::PortMapping::send_from_queue() {
  MOZ_ASSERT(remote_address_.protocol != IPPROTO_TCP);
  int r = 0;

  while (!send_queue_.empty()) {
    UdpPacket &packet = *send_queue_.front();
    r_log(LOG_GENERIC, LOG_INFO,
          "PortMapping %s -> %s sending from queue to %s",
          external_socket_->my_addr().as_string,
          remote_address_.as_string,
          packet.remote_address_.as_string);

    r = external_socket_->sendto(packet.buffer_->data(),
                                 packet.buffer_->len(),
                                 0,
                                 &packet.remote_address_);

    if (r) {
      if (r != R_WOULDBLOCK) {
        r_log(LOG_GENERIC, LOG_ERR, "%s: Fatal error %d, stop trying",
              __FUNCTION__, r);
        send_queue_.clear();
      } else {
        r_log(LOG_GENERIC, LOG_INFO, "Would block, will retry later");
      }
      break;
    }

    send_queue_.pop_front();
  }

  return r;
}

int TestNrSocket::PortMapping::sendto(const void *msg,
                                      size_t len,
                                      const nr_transport_addr &to) {
  MOZ_ASSERT(remote_address_.protocol != IPPROTO_TCP);
  r_log(LOG_GENERIC, LOG_INFO,
        "PortMapping %s -> %s sending to %s",
        external_socket_->my_addr().as_string,
        remote_address_.as_string,
        to.as_string);

  last_used_ = PR_IntervalNow();
  int r = external_socket_->sendto(msg, len, 0,
      
      const_cast<nr_transport_addr*>(&to));

  if (r == R_WOULDBLOCK) {
    r_log(LOG_GENERIC, LOG_INFO, "Enqueueing UDP packet to %s", to.as_string);
    send_queue_.push_back(nsRefPtr<UdpPacket>(new UdpPacket(msg, len, to)));
    return 0;
  } else if (r) {
    r_log(LOG_GENERIC,LOG_ERR, "Error: %d", r);
  }

  return r;
}

int TestNrSocket::PortMapping::async_wait(int how, NR_async_cb cb, void *cb_arg,
                                          char *function, int line) {
  r_log(LOG_GENERIC, LOG_DEBUG,
        "PortMapping %s -> %s waiting for %s",
        external_socket_->my_addr().as_string,
        remote_address_.as_string,
        how == NR_ASYNC_WAIT_READ ? "read" : "write");

  return external_socket_->async_wait(how, cb, cb_arg, function, line);
}

int TestNrSocket::PortMapping::cancel(int how) {
  r_log(LOG_GENERIC, LOG_DEBUG,
        "PortMapping %s -> %s stop waiting for %s",
        external_socket_->my_addr().as_string,
        remote_address_.as_string,
        how == NR_ASYNC_WAIT_READ ? "read" : "write");

  return external_socket_->cancel(how);
}
} 

