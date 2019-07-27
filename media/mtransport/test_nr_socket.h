

















































































#ifndef test_nr_socket__
#define test_nr_socket__

#include "nr_socket_prsock.h"

extern "C" {
#include "nr_socket.h"
}

#include <set>
#include <vector>
#include <map>
#include <list>

#include "mozilla/UniquePtr.h"
#include "prinrval.h"

namespace mozilla {

class TestNrSocket;






class TestNat {
  public:
    typedef enum {
      

      ENDPOINT_INDEPENDENT,

      


      ADDRESS_DEPENDENT,

      


      PORT_DEPENDENT,
    } NatBehavior;

    TestNat() :
      enabled_(false),
      filtering_type_(ENDPOINT_INDEPENDENT),
      mapping_type_(ENDPOINT_INDEPENDENT),
      mapping_timeout_(30000),
      allow_hairpinning_(false),
      refresh_on_ingress_(false),
      block_udp_(false),
      sockets_() {}

    bool has_port_mappings() const;

    
    bool is_my_external_tuple(const nr_transport_addr &addr) const;
    bool is_an_internal_tuple(const nr_transport_addr &addr) const;

    int create_socket_factory(nr_socket_factory **factorypp);

    void insert_socket(TestNrSocket *socket) {
      sockets_.insert(socket);
    }

    void erase_socket(TestNrSocket *socket) {
      sockets_.erase(socket);
    }

    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(TestNat);

    bool enabled_;
    TestNat::NatBehavior filtering_type_;
    TestNat::NatBehavior mapping_type_;
    uint32_t mapping_timeout_;
    bool allow_hairpinning_;
    bool refresh_on_ingress_;
    bool block_udp_;

  private:
    std::set<TestNrSocket*> sockets_;

    ~TestNat(){}
};






class TestNrSocket : public NrSocket {
  public:
    explicit TestNrSocket(TestNat *nat);

    virtual ~TestNrSocket();

    bool has_port_mappings() const;
    bool is_my_external_tuple(const nr_transport_addr &addr) const;

    
    int sendto(const void *msg, size_t len,
               int flags, nr_transport_addr *to) override;
    int recvfrom(void * buf, size_t maxlen,
                 size_t *len, int flags,
                 nr_transport_addr *from) override;
    int connect(nr_transport_addr *addr) override;
    int write(const void *msg, size_t len, size_t *written) override;
    int read(void *buf, size_t maxlen, size_t *len) override;

    int async_wait(int how, NR_async_cb cb, void *cb_arg,
                   char *function, int line) override;
    int cancel(int how) override;

  private:
    class UdpPacket {
      public:
        UdpPacket(const void *msg, size_t len, const nr_transport_addr &addr) :
          buffer_(new DataBuffer(static_cast<const uint8_t*>(msg), len)) {
          
          nr_transport_addr_copy(&remote_address_,
                                 const_cast<nr_transport_addr*>(&addr));
        }

        nr_transport_addr remote_address_;
        UniquePtr<DataBuffer> buffer_;

        NS_INLINE_DECL_THREADSAFE_REFCOUNTING(UdpPacket);
      private:
        ~UdpPacket(){}
    };

    class PortMapping {
      public:
        PortMapping(const nr_transport_addr &remote_address,
                    const nsRefPtr<NrSocket> &external_socket);

        int sendto(const void *msg, size_t len, const nr_transport_addr &to);
        int async_wait(int how, NR_async_cb cb, void *cb_arg,
                       char *function, int line);
        int cancel(int how);
        int send_from_queue();
        NS_INLINE_DECL_THREADSAFE_REFCOUNTING(PortMapping);

        PRIntervalTime last_used_;
        nsRefPtr<NrSocket> external_socket_;
        
        nr_transport_addr remote_address_;

      private:
        ~PortMapping(){}

        
        
        
        std::list<nsRefPtr<UdpPacket>> send_queue_;
    };

    bool is_port_mapping_stale(const PortMapping &port_mapping) const;
    bool allow_ingress(const nr_transport_addr &from,
                       PortMapping **port_mapping_used) const;
    void destroy_stale_port_mappings();

    static void port_mapping_readable_callback(void *ext_sock_v,
                                               int how,
                                               void *test_sock_v);
    void on_port_mapping_readable(NrSocket *external_socket);
    void fire_readable_callback();

    static void port_mapping_tcp_passthrough_callback(void *ext_sock_v,
                                                      int how,
                                                      void *test_sock_v);
    void cancel_port_mapping_async_wait(int how);

    static void port_mapping_writeable_callback(void *ext_sock_v,
                                                int how,
                                                void *test_sock_v);
    void write_to_port_mapping(NrSocket *external_socket);
    bool is_tcp_connection_behind_nat() const;

    PortMapping* get_port_mapping(const nr_transport_addr &remote_addr,
                                  TestNat::NatBehavior filter) const;
    PortMapping* create_port_mapping(
        const nr_transport_addr &remote_addr,
        const nsRefPtr<NrSocket> &external_socket) const;
    nsRefPtr<NrSocket> create_external_socket(
        const nr_transport_addr &remote_addr) const;

    nsRefPtr<NrSocket> readable_socket_;
    nsRefPtr<TestNat> nat_;
    
    
    
    
    std::list<nsRefPtr<PortMapping>> port_mappings_;
};

} 

#endif 

