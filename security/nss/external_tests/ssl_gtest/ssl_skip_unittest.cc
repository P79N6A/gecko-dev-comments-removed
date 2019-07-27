





#include "sslerr.h"

#include "tls_parser.h"
#include "tls_filter.h"
#include "tls_connect.h"








namespace nss_test {

class TlsHandshakeSkipFilter : public TlsRecordFilter {
 public:
  
  TlsHandshakeSkipFilter(uint8_t handshake_type)
      : handshake_type_(handshake_type),
        skipped_(false) {}

 protected:
  
  
  virtual bool FilterRecord(uint8_t content_type, uint16_t version,
                            const DataBuffer& input, DataBuffer* output) {
    if (content_type != kTlsHandshakeType) {
      return false;
    }

    size_t output_offset = 0U;
    output->Allocate(input.len());

    TlsParser parser(input);
    while (parser.remaining()) {
      size_t start = parser.consumed();
      uint8_t handshake_type;
      if (!parser.Read(&handshake_type)) {
        return false;
      }
      uint32_t length;
      if (!TlsHandshakeFilter::ReadLength(&parser, version, &length)) {
        return false;
      }

      if (!parser.Skip(length)) {
        return false;
      }

      if (skipped_ || handshake_type != handshake_type_) {
        size_t entire_length = parser.consumed() - start;
        output->Write(output_offset, input.data() + start,
                      entire_length);
        
        if (skipped_ && IsDtls(version)) {
          output->data()[start + 5] -= 1;
        }
        output_offset += entire_length;
      } else {
        std::cerr << "Dropping handshake: "
                  << static_cast<unsigned>(handshake_type_) << std::endl;
        
        
        
        
        skipped_ = true;
      }
    }
    output->Truncate(output_offset);
    return skipped_;
  }

 private:
  
  uint8_t handshake_type_;
  
  
  
  bool skipped_;
};

class TlsSkipTest
  : public TlsConnectTestBase,
    public ::testing::WithParamInterface<std::tuple<std::string, uint16_t>> {

 protected:
  TlsSkipTest()
    : TlsConnectTestBase(TlsConnectTestBase::ToMode(std::get<0>(GetParam())),
                         std::get<1>(GetParam())) {}

  void ServerSkipTest(PacketFilter* filter,
                      uint8_t alert = kTlsAlertUnexpectedMessage) {
    auto alert_recorder = new TlsAlertRecorder();
    client_->SetPacketFilter(alert_recorder);
    if (filter) {
      server_->SetPacketFilter(filter);
    }
    ConnectExpectFail();
    EXPECT_EQ(kTlsAlertFatal, alert_recorder->level());
    EXPECT_EQ(alert, alert_recorder->description());
  }
};

TEST_P(TlsSkipTest, SkipCertificate) {
  ServerSkipTest(new TlsHandshakeSkipFilter(kTlsHandshakeCertificate));
  client_->CheckErrorCode(SSL_ERROR_RX_UNEXPECTED_HELLO_DONE);
}

TEST_P(TlsSkipTest, SkipCertificateEcdhe) {
  EnableSomeEcdheCiphers();
  ServerSkipTest(new TlsHandshakeSkipFilter(kTlsHandshakeCertificate));
  client_->CheckErrorCode(SSL_ERROR_RX_UNEXPECTED_SERVER_KEY_EXCH);
}

TEST_P(TlsSkipTest, SkipCertificateEcdsa) {
  ResetEcdsa();
  ServerSkipTest(new TlsHandshakeSkipFilter(kTlsHandshakeCertificate));
  client_->CheckErrorCode(SSL_ERROR_RX_UNEXPECTED_SERVER_KEY_EXCH);
}

TEST_P(TlsSkipTest, SkipServerKeyExchange) {
  
  EnableSomeEcdheCiphers();
  ServerSkipTest(new TlsHandshakeSkipFilter(kTlsHandshakeServerKeyExchange));
  client_->CheckErrorCode(SSL_ERROR_RX_UNEXPECTED_HELLO_DONE);
}

TEST_P(TlsSkipTest, SkipServerKeyExchangeEcdsa) {
  ResetEcdsa();
  ServerSkipTest(new TlsHandshakeSkipFilter(kTlsHandshakeServerKeyExchange));
  client_->CheckErrorCode(SSL_ERROR_RX_UNEXPECTED_HELLO_DONE);
}

TEST_P(TlsSkipTest, SkipCertAndKeyExch) {
  auto chain = new ChainedPacketFilter();
  chain->Add(new TlsHandshakeSkipFilter(kTlsHandshakeCertificate));
  chain->Add(new TlsHandshakeSkipFilter(kTlsHandshakeServerKeyExchange));
  ServerSkipTest(chain);
  client_->CheckErrorCode(SSL_ERROR_RX_UNEXPECTED_HELLO_DONE);
}

TEST_P(TlsSkipTest, SkipCertAndKeyExchEcdsa) {
  ResetEcdsa();
  auto chain = new ChainedPacketFilter();
  chain->Add(new TlsHandshakeSkipFilter(kTlsHandshakeCertificate));
  chain->Add(new TlsHandshakeSkipFilter(kTlsHandshakeServerKeyExchange));
  ServerSkipTest(chain);
  client_->CheckErrorCode(SSL_ERROR_RX_UNEXPECTED_HELLO_DONE);
}

INSTANTIATE_TEST_CASE_P(SkipTls10, TlsSkipTest,
                        ::testing::Combine(
                          TlsConnectTestBase::kTlsModesStream,
                          TlsConnectTestBase::kTlsV10));
INSTANTIATE_TEST_CASE_P(SkipVariants, TlsSkipTest,
                        ::testing::Combine(
                          TlsConnectTestBase::kTlsModesAll,
                          TlsConnectTestBase::kTlsV11V12));

}  
