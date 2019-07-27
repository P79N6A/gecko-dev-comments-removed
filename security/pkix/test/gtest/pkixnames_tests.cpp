























#include <cstring>

#include "pkix/Input.h"
#include "pkixgtest.h"
#include "pkixtestutil.h"

namespace mozilla { namespace pkix {

bool PresentedDNSIDMatchesReferenceDNSID(Input presentedDNSID,
                                         Input referenceDNSID,
                                         bool referenceDNSIDWasVerifiedAsValid);

bool IsValidDNSName(Input hostname);
bool ParseIPv4Address(Input hostname,  uint8_t (&out)[4]);
bool ParseIPv6Address(Input hostname,  uint8_t (&out)[16]);

} } 

using namespace mozilla::pkix;
using namespace mozilla::pkix::test;

struct PresentedMatchesReference
{
  ByteString presentedDNSID;
  ByteString referenceDNSID;
  bool matches;
};

#define DNS_ID_MATCH(a, b) \
  { \
    ByteString(reinterpret_cast<const uint8_t*>(a), sizeof(a) - 1), \
    ByteString(reinterpret_cast<const uint8_t*>(b), sizeof(b) - 1), \
    true \
  }

#define DNS_ID_MISMATCH(a, b) \
  { \
    ByteString(reinterpret_cast<const uint8_t*>(a), sizeof(a) - 1), \
    ByteString(reinterpret_cast<const uint8_t*>(b), sizeof(b) - 1), \
    false \
  }

static const PresentedMatchesReference DNSID_MATCH_PARAMS[] =
{
  DNS_ID_MISMATCH("", "a"),

  DNS_ID_MATCH("a", "a"),
  DNS_ID_MISMATCH("b", "a"),

  DNS_ID_MATCH("*.b.a", "c.b.a"),
  DNS_ID_MISMATCH("*.b.a", "b.a"),
  DNS_ID_MISMATCH("*.b.a", "b.a."),

  
  DNS_ID_MATCH("d.c.b.a", "d.c.b.a"),
  DNS_ID_MISMATCH("d.*.b.a", "d.c.b.a"),
  DNS_ID_MISMATCH("d.c*.b.a", "d.c.b.a"),
  DNS_ID_MISMATCH("d.c*.b.a", "d.cc.b.a"),

  
  DNS_ID_MATCH("abcdefghijklmnopqrstuvwxyz", "ABCDEFGHIJKLMNOPQRSTUVWXYZ"),
  DNS_ID_MATCH("ABCDEFGHIJKLMNOPQRSTUVWXYZ", "abcdefghijklmnopqrstuvwxyz"),
  DNS_ID_MATCH("aBc", "Abc"),

  
  DNS_ID_MATCH("a1", "a1"),

  
  
  DNS_ID_MATCH("example", "example"),
  DNS_ID_MATCH("example.", "example."),
  DNS_ID_MATCH("example", "example."),
  DNS_ID_MATCH("example.", "example"),
  DNS_ID_MATCH("example.com", "example.com"),
  DNS_ID_MATCH("example.com.", "example.com."),
  DNS_ID_MATCH("example.com", "example.com."),
  DNS_ID_MATCH("example.com.", "example.com"),
  DNS_ID_MISMATCH("example.com..", "example.com."),
  DNS_ID_MISMATCH("example.com..", "example.com"),
  DNS_ID_MISMATCH("example.com...", "example.com."),

  
  DNS_ID_MATCH("x*.b.a", "xa.b.a"),
  DNS_ID_MATCH("x*.b.a", "xna.b.a"),
  DNS_ID_MATCH("x*.b.a", "xn-a.b.a"),
  DNS_ID_MISMATCH("x*.b.a", "xn--a.b.a"),
  DNS_ID_MISMATCH("xn*.b.a", "xn--a.b.a"),
  DNS_ID_MISMATCH("xn-*.b.a", "xn--a.b.a"),
  DNS_ID_MISMATCH("xn--*.b.a", "xn--a.b.a"),
  DNS_ID_MISMATCH("xn*.b.a", "xn--a.b.a"),
  DNS_ID_MISMATCH("xn-*.b.a", "xn--a.b.a"),
  DNS_ID_MISMATCH("xn--*.b.a", "xn--a.b.a"),
  DNS_ID_MISMATCH("xn---*.b.a", "xn--a.b.a"),

  
  DNS_ID_MISMATCH("c*.b.a", "c.b.a"),

  
  
  
  
  
  

  DNS_ID_MATCH("foo.com", "foo.com"),
  DNS_ID_MATCH("f", "f"),
  DNS_ID_MISMATCH("i", "h"),
  DNS_ID_MATCH("*.foo.com", "bar.foo.com"),
  DNS_ID_MATCH("*.test.fr", "www.test.fr"),
  DNS_ID_MATCH("*.test.FR", "wwW.tESt.fr"),
  DNS_ID_MISMATCH(".uk", "f.uk"),
  DNS_ID_MISMATCH("?.bar.foo.com", "w.bar.foo.com"),
  DNS_ID_MISMATCH("(www|ftp).foo.com", "www.foo.com"), 
  DNS_ID_MISMATCH("www.foo.com\0", "www.foo.com"),
  DNS_ID_MISMATCH("www.foo.com\0*.foo.com", "www.foo.com"),
  DNS_ID_MISMATCH("ww.house.example", "www.house.example"),
  DNS_ID_MISMATCH("www.test.org", "test.org"),
  DNS_ID_MISMATCH("*.test.org", "test.org"),
  DNS_ID_MISMATCH("*.org", "test.org"),
  DNS_ID_MISMATCH("w*.bar.foo.com", "w.bar.foo.com"),
  DNS_ID_MISMATCH("ww*ww.bar.foo.com", "www.bar.foo.com"),
  DNS_ID_MISMATCH("ww*ww.bar.foo.com", "wwww.bar.foo.com"),

  
  DNS_ID_MISMATCH("w*w.bar.foo.com", "wwww.bar.foo.com"),

  DNS_ID_MISMATCH("w*w.bar.foo.c0m", "wwww.bar.foo.com"),

  DNS_ID_MATCH("wa*.bar.foo.com", "WALLY.bar.foo.com"),

  
  
  DNS_ID_MISMATCH("*Ly.bar.foo.com", "wally.bar.foo.com"),

  
  
  
  

  DNS_ID_MISMATCH("*.test.de", "www.test.co.jp"),
  DNS_ID_MISMATCH("*.jp", "www.test.co.jp"),
  DNS_ID_MISMATCH("www.test.co.uk", "www.test.co.jp"),
  DNS_ID_MISMATCH("www.*.co.jp", "www.test.co.jp"),
  DNS_ID_MATCH("www.bar.foo.com", "www.bar.foo.com"),
  DNS_ID_MISMATCH("*.foo.com", "www.bar.foo.com"),
  DNS_ID_MISMATCH("*.*.foo.com", "www.bar.foo.com"),
  DNS_ID_MISMATCH("*.*.foo.com", "www.bar.foo.com"),

  
  
  

  DNS_ID_MATCH("www.bath.org", "www.bath.org"),

  
  
  
  
  

  
  DNS_ID_MATCH("xn--poema-9qae5a.com.br", "xn--poema-9qae5a.com.br"),
  DNS_ID_MATCH("*.xn--poema-9qae5a.com.br", "www.xn--poema-9qae5a.com.br"),
  DNS_ID_MISMATCH("*.xn--poema-9qae5a.com.br", "xn--poema-9qae5a.com.br"),
  DNS_ID_MISMATCH("xn--poema-*.com.br", "xn--poema-9qae5a.com.br"),
  DNS_ID_MISMATCH("xn--*-9qae5a.com.br", "xn--poema-9qae5a.com.br"),
  DNS_ID_MISMATCH("*--poema-9qae5a.com.br", "xn--poema-9qae5a.com.br"),

  
  
  
  
  DNS_ID_MATCH("*.example.com", "foo.example.com"),
  DNS_ID_MISMATCH("*.example.com", "bar.foo.example.com"),
  DNS_ID_MISMATCH("*.example.com", "example.com"),
  
  
  
  DNS_ID_MATCH("baz*.example.net", "baz1.example.net"),

  
  
  DNS_ID_MISMATCH("*baz.example.net", "foobaz.example.net"),
  DNS_ID_MISMATCH("b*z.example.net", "buzz.example.net"),

  
  
  
  
  DNS_ID_MATCH("*.test.example", "www.test.example"),
  DNS_ID_MATCH("*.example.co.uk", "test.example.co.uk"),
  DNS_ID_MISMATCH("*.exmaple", "test.example"),

  
  
  
  DNS_ID_MATCH("*.co.uk", "example.co.uk"),

  DNS_ID_MISMATCH("*.com", "foo.com"),
  DNS_ID_MISMATCH("*.us", "foo.us"),
  DNS_ID_MISMATCH("*", "foo"),

  
  DNS_ID_MATCH("*.xn--poema-9qae5a.com.br", "www.xn--poema-9qae5a.com.br"),
  DNS_ID_MATCH("*.example.xn--mgbaam7a8h", "test.example.xn--mgbaam7a8h"),

  
  
  DNS_ID_MATCH("*.com.br", "xn--poema-9qae5a.com.br"),

  DNS_ID_MISMATCH("*.xn--mgbaam7a8h", "example.xn--mgbaam7a8h"),
  
  
  
  DNS_ID_MATCH("*.appspot.com", "www.appspot.com"),
  DNS_ID_MATCH("*.s3.amazonaws.com", "foo.s3.amazonaws.com"),

  
  DNS_ID_MISMATCH("*.*.com", "foo.example.com"),
  DNS_ID_MISMATCH("*.bar.*.com", "foo.bar.example.com"),

  
  
  
  
  DNS_ID_MATCH("foo.com.", "foo.com"),
  DNS_ID_MATCH("foo.com", "foo.com."),
  DNS_ID_MATCH("foo.com.", "foo.com."),
  DNS_ID_MATCH("f.", "f"),
  DNS_ID_MATCH("f", "f."),
  DNS_ID_MATCH("f.", "f."),
  DNS_ID_MATCH("*.bar.foo.com.", "www-3.bar.foo.com"),
  DNS_ID_MATCH("*.bar.foo.com", "www-3.bar.foo.com."),
  DNS_ID_MATCH("*.bar.foo.com.", "www-3.bar.foo.com."),

  
  
  

  DNS_ID_MISMATCH("*.com.", "example.com"),
  DNS_ID_MISMATCH("*.com", "example.com."),
  DNS_ID_MISMATCH("*.com.", "example.com."),
  DNS_ID_MISMATCH("*.", "foo."),
  DNS_ID_MISMATCH("*.", "foo"),

  
  
  DNS_ID_MATCH("*.co.uk.", "foo.co.uk"),
  DNS_ID_MATCH("*.co.uk.", "foo.co.uk."),
};

struct InputValidity
{
  ByteString input;
  bool isValid;
};



#define I(str, valid) \
  { \
    ByteString(reinterpret_cast<const uint8_t*>(str), sizeof(str) - 1), valid \
  }

static const InputValidity DNSNAMES_VALIDITY[] =
{
  I("a", true),
  I("a.b", true),
  I("a.b.c", true),
  I("a.b.c.d", true),

  
  I("", false),
  I(".", false),
  I("a", true),
  I(".a", false), 
  I(".a.b", false), 
  I("..a", false),
  I("a..b", false),
  I("a...b", false),
  I("a..b.c", false),
  I("a.b..c", false),
  I(".a.b.c.", false),

  
  I("a.", true),
  I("a.b.", true),
  I("a.b.c.", true),

  
  I("a..", false),
  I("a.b..", false),
  I("a.b.c..", false),
  I("a...", false),

  
  I("xn--", false),
  I("xn--.", false),
  I("xn--.a", false),
  I("a.xn--", false),
  I("a.xn--.", false),
  I("a.xn--.b", false),
  I("a.xn--.b", false),
  I("a.xn--\0.b", false),
  I("a.xn--a.b", true),
  I("xn--a", true),
  I("a.xn--a", true),
  I("a.xn--a.a", true),
  I("\0xc4\0x95.com", false), 
  I("xn--jea.com", true), 
  I("xn--\0xc4\0x95.com", false), 

  
  I("xn--google.com", true), 
  I("xn--citibank.com", true), 
  I("xn--cnn.com", true), 
  I("a.xn--cnn", true), 
  I("a.xn--cnn.com", true), 

  I("1.2.3.4", false), 
  I("1::2", false), 

  
  I(" ", false),
  I(" a", false),
  I("a ", false),
  I("a b", false),
  I("a.b 1", false),
  I("a\t", false),

  
  I("\0", false),
  I("a\0", false),
  I("example.org\0.example.com", false), 
  I("\0a", false),
  I("xn--\0", false),

  
  I("a.b.c.d.e.f.g.h.i.j.k.l.m.n.o.p.q.r.s.t.u.v.w.x.y.z", true),
  I("A.B.C.D.E.F.G.H.I.J.K.L.M.N.O.P.Q.R.S.T.U.V.W.X.Y.Z", true),
  I("0.1.2.3.4.5.6.7.8.9.a", true), 
  I("a-b", true), 

  
  I("!", false),
  I("!a", false),
  I("a!", false),
  I("a!b", false),
  I("a.!", false),
  I("a.a!", false),
  I("a.!a", false),
  I("a.a!a", false),
  I("a.!a.a", false),
  I("a.a!.a", false),
  I("a.a!a.a", false),

  
  I("a!", false),
  I("a@", false),
  I("a#", false),
  I("a$", false),
  I("a%", false),
  I("a^", false),
  I("a&", false),
  I("a*", false),
  I("a(", false),
  I("a)", false),

  
  I("1", false),
  I("a.1", false),

  
  I("1.a", true),
  I("1.2.a", true),
  I("1.2.3.a", true),

  
  I("1a", true),
  I("1.1a", true),
  I("1-1", true),
  I("a.1-1", true),
  I("a.1-a", true),

  
  I("-", false),
  I("-1", false),

  
  I("1-", false),
  I("1-.a", false),
  I("a-", false),
  I("a-.a", false),
  I("a.1-.a", false),
  I("a.a-.a", false),

  
  I("a-b", true),
  I("1-2", true),
  I("a.a-1", true),

  
  I("a--1", true),
  I("1---a", true),
  I("a-----------------b", true),

  
  I("*.a", false),
  I("a*", false),
  I("a*.a", false),

  
  
  I("(PRIVATE).foo", false),

  
  I("1234567890" "1234567890" "1234567890"
    "1234567890" "1234567890" "1234567890" "abc", true),
  I("1234567890" "1234567890" "1234567890"
    "1234567890" "1234567890" "1234567890" "abcd", false),

  
  I("1234567890" "1234567890" "1234567890" "1234567890" "1234567890" "."
    "1234567890" "1234567890" "1234567890" "1234567890" "1234567890" "."
    "1234567890" "1234567890" "1234567890" "1234567890" "1234567890" "."
    "1234567890" "1234567890" "1234567890" "1234567890" "1234567890" "."
    "1234567890" "1234567890" "1234567890" "1234567890" "12345678" "a",
    true),
  I("1234567890" "1234567890" "1234567890" "1234567890" "1234567890" "."
    "1234567890" "1234567890" "1234567890" "1234567890" "1234567890" "."
    "1234567890" "1234567890" "1234567890" "1234567890" "1234567890" "."
    "1234567890" "1234567890" "1234567890" "1234567890" "1234567890" "."
    "1234567890" "1234567890" "1234567890" "1234567890" "123456789" "a",
    false),
};

static const InputValidity DNSNAMES_VALIDITY_TURKISH_I[] =
{
  
  
  
  I("I", true), 
  I("i", true), 
  I("\0xC4\0xB0", false), 
  I("\0xC4\0xB1", false), 
  I("xn--i-9bb", true), 
  I("xn--cfa", true), 
  I("xn--\0xC4\0xB0", false), 
  I("xn--\0xC4\0xB1", false), 
};

static const uint8_t LOWERCASE_I_VALUE[1] = { 'i' };
static const uint8_t UPPERCASE_I_VALUE[1] = { 'I' };
static const Input LOWERCASE_I(LOWERCASE_I_VALUE);
static const Input UPPERCASE_I(UPPERCASE_I_VALUE);

template <unsigned int L>
struct IPAddressParams
{
  ByteString input;
  bool isValid;
  uint8_t expectedValueIfValid[L];
};

#define IPV4_VALID(str, a, b, c, d) \
  { \
    ByteString(reinterpret_cast<const uint8_t*>(str), sizeof(str) - 1), \
    true, \
    { a, b, c, d } \
  }




#define IPV4_INVALID(str) \
  { \
    ByteString(reinterpret_cast<const uint8_t*>(str), sizeof(str) - 1), \
    false, \
    { 73, 73, 73, 73 } \
  }

static const IPAddressParams<4> IPV4_ADDRESSES[] =
{
  IPV4_INVALID(""),
  IPV4_INVALID("1"),
  IPV4_INVALID("1.2"),
  IPV4_INVALID("1.2.3"),
  IPV4_VALID("1.2.3.4", 1, 2, 3, 4),
  IPV4_INVALID("1.2.3.4.5"),

  IPV4_INVALID("1.2.3.4a"), 
  IPV4_INVALID("a.2.3.4"), 
  IPV4_INVALID("1::2"), 

  
  IPV4_INVALID(" 1.2.3.4"),
  IPV4_INVALID("1.2.3.4 "),
  IPV4_INVALID("1 .2.3.4"),
  IPV4_INVALID("\n1.2.3.4"),
  IPV4_INVALID("1.2.3.4\n"),

  
  IPV4_INVALID("\0"),
  IPV4_INVALID("\0" "1.2.3.4"),
  IPV4_INVALID("1.2.3.4\0"),
  IPV4_INVALID("1.2.3.4\0.5"),

  
  IPV4_VALID("0.0.0.0", 0, 0, 0, 0),
  IPV4_VALID("255.255.255.255", 255, 255, 255, 255),
  IPV4_INVALID("256.0.0.0"),
  IPV4_INVALID("0.256.0.0"),
  IPV4_INVALID("0.0.256.0"),
  IPV4_INVALID("0.0.0.256"),
  IPV4_INVALID("999.0.0.0"),
  IPV4_INVALID("9999999999999999999.0.0.0"),

  
  IPV4_VALID("0.1.2.3", 0, 1, 2, 3),
  IPV4_VALID("4.5.6.7", 4, 5, 6, 7),
  IPV4_VALID("8.9.0.1", 8, 9, 0, 1),

  
  IPV4_INVALID("01.2.3.4"),
  IPV4_INVALID("001.2.3.4"),
  IPV4_INVALID("00000000001.2.3.4"),
  IPV4_INVALID("010.2.3.4"),
  IPV4_INVALID("1.02.3.4"),
  IPV4_INVALID("1.2.03.4"),
  IPV4_INVALID("1.2.3.04"),

  
  IPV4_INVALID(".2.3.4"),
  IPV4_INVALID("1..3.4"),
  IPV4_INVALID("1.2..4"),
  IPV4_INVALID("1.2.3."),

  
  IPV4_INVALID("1.2.3.4.5"),
  IPV4_INVALID("1.2.3.4.5.6"),
  IPV4_INVALID("0.1.2.3.4"),
  IPV4_INVALID("1.2.3.4.0"),

  
  IPV4_INVALID(".1.2.3.4"),
  IPV4_INVALID("1.2.3.4."),

  
  
  IPV4_VALID("192.0.2.235", 192, 0, 2, 235), 
  IPV4_INVALID("0xC0.0x00.0x02.0xEB"), 
  IPV4_INVALID("0301.0000.0002.0353"), 
  IPV4_INVALID("0xC00002EB"), 
  IPV4_INVALID("3221226219"), 
  IPV4_INVALID("030000001353"), 
  IPV4_INVALID("192.0.0002.0xEB"), 
};

#define IPV6_VALID(str, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) \
  { \
    ByteString(reinterpret_cast<const uint8_t*>(str), sizeof(str) - 1), \
    true, \
    { a, b, c, d, \
      e, f, g, h, \
      i, j, k, l, \
      m, n, o, p } \
  }

#define IPV6_INVALID(str) \
  { \
    ByteString(reinterpret_cast<const uint8_t*>(str), sizeof(str) - 1), \
    false, \
    { 73, 73, 73, 73, \
      73, 73, 73, 73, \
      73, 73, 73, 73, \
      73, 73, 73, 73 } \
  }

static const IPAddressParams<16> IPV6_ADDRESSES[] =
{
  IPV6_INVALID(""),
  IPV6_INVALID("1234"),
  IPV6_INVALID("1234:5678"),
  IPV6_INVALID("1234:5678:9abc"),
  IPV6_INVALID("1234:5678:9abc:def0"),
  IPV6_INVALID("1234:5678:9abc:def0:1234:"),
  IPV6_INVALID("1234:5678:9abc:def0:1234:5678:"),
  IPV6_INVALID("1234:5678:9abc:def0:1234:5678:9abc:"),
  IPV6_VALID("1234:5678:9abc:def0:1234:5678:9abc:def0",
             0x12, 0x34, 0x56, 0x78,
             0x9a, 0xbc, 0xde, 0xf0,
             0x12, 0x34, 0x56, 0x78,
             0x9a, 0xbc, 0xde, 0xf0),
  IPV6_INVALID("1234:5678:9abc:def0:1234:5678:9abc:def0:"),
  IPV6_INVALID(":1234:5678:9abc:def0:1234:5678:9abc:def0"),
  IPV6_INVALID("1234:5678:9abc:def0:1234:5678:9abc:def0:0000"),

  
  IPV6_VALID("::1",
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x01),
  IPV6_VALID("::1234",
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x12, 0x34),
  IPV6_VALID("1234::",
             0x12, 0x34, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00),
  IPV6_VALID("1234::5678",
             0x12, 0x34, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x56, 0x78),
  IPV6_VALID("1234:5678::abcd",
             0x12, 0x34, 0x56, 0x78,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0xab, 0xcd),
  IPV6_VALID("1234:5678:9abc:def0:1234:5678:9abc::",
             0x12, 0x34, 0x56, 0x78,
             0x9a, 0xbc, 0xde, 0xf0,
             0x12, 0x34, 0x56, 0x78,
             0x9a, 0xbc, 0x00, 0x00),

  
  IPV6_INVALID("::1234:5678:9abc:def0:1234:5678:9abc:def0"), 
  IPV6_INVALID("1234:5678:9abc:def0:1234:5678:9abc:def0::"), 
  IPV6_INVALID("1234:5678::9abc:def0:1234:5678:9abc:def0"), 

  
  IPV6_INVALID("::1::"),
  IPV6_INVALID("::1::2"),
  IPV6_INVALID("1::2::"),

  
  IPV6_INVALID(":"),
  IPV6_INVALID("::"),
  IPV6_INVALID(":::"),
  IPV6_INVALID("::::"),
  IPV6_INVALID(":::1"),
  IPV6_INVALID("::::1"),
  IPV6_INVALID("1:::2"),
  IPV6_INVALID("1::::2"),
  IPV6_INVALID("1:2:::"),
  IPV6_INVALID("1:2::::"),
  IPV6_INVALID("::1234:"),
  IPV6_INVALID(":1234::"),

  IPV6_INVALID("01234::"), 
  IPV6_INVALID("12345678::"), 

  
  IPV6_VALID("ABCD:EFAB::",
             0xab, 0xcd, 0xef, 0xab,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00),

  
  IPV6_VALID("aBcd:eFAb::",
             0xab, 0xcd, 0xef, 0xab,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00),

  
  IPV6_VALID("::2.3.4.5",
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x02, 0x03, 0x04, 0x05),
  IPV6_VALID("1234::2.3.4.5",
             0x12, 0x34, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x02, 0x03, 0x04, 0x05),
  IPV6_VALID("::abcd:2.3.4.5",
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0xab, 0xcd,
             0x02, 0x03, 0x04, 0x05),
  IPV6_VALID("1234:5678:9abc:def0:1234:5678:252.253.254.255",
             0x12, 0x34, 0x56, 0x78,
             0x9a, 0xbc, 0xde, 0xf0,
             0x12, 0x34, 0x56, 0x78,
             252,  253,  254,  255),
  IPV6_VALID("1234:5678:9abc:def0:1234::252.253.254.255",
             0x12, 0x34, 0x56, 0x78,
             0x9a, 0xbc, 0xde, 0xf0,
             0x12, 0x34, 0x00, 0x00,
             252,  253,  254,  255),
  IPV6_INVALID("1234::252.253.254"),
  IPV6_INVALID("::252.253.254"),
  IPV6_INVALID("::252.253.254.300"),
  IPV6_INVALID("1234::252.253.254.255:"),
  IPV6_INVALID("1234::252.253.254.255:5678"),

  
  IPV6_INVALID("::1234:5678:9abc:def0:1234:5678:9abc:def0"),
  IPV6_INVALID("1234:5678:9abc:def0:1234:5678:9abc:def0::"),
  IPV6_INVALID("1234:5678:9abc:def0::1234:5678:9abc:def0"),
  IPV6_INVALID("1234:5678:9abc:def0:1234:5678::252.253.254.255"),

  
  IPV6_VALID("::123",
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x01, 0x23),
  IPV6_VALID("::0123",
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x01, 0x23),
  IPV6_VALID("::012",
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x12),
  IPV6_VALID("::0012",
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x12),
  IPV6_VALID("::01",
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x01),
  IPV6_VALID("::001",
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x01),
  IPV6_VALID("::0001",
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x01),
  IPV6_VALID("::0",
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00),
  IPV6_VALID("::00",
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00),
  IPV6_VALID("::000",
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00),
  IPV6_VALID("::0000",
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00),
  IPV6_INVALID("::01234"),
  IPV6_INVALID("::00123"),
  IPV6_INVALID("::000123"),

  
  IPV6_INVALID("::12340"),

  
  IPV6_INVALID(" 1234:5678:9abc:def0:1234:5678:9abc:def0"),
  IPV6_INVALID("\t1234:5678:9abc:def0:1234:5678:9abc:def0"),
  IPV6_INVALID("\t1234:5678:9abc:def0:1234:5678:9abc:def0\n"),
  IPV6_INVALID("1234 :5678:9abc:def0:1234:5678:9abc:def0"),
  IPV6_INVALID("1234: 5678:9abc:def0:1234:5678:9abc:def0"),
  IPV6_INVALID(":: 2.3.4.5"),
  IPV6_INVALID("1234::252.253.254.255 "),
  IPV6_INVALID("1234::252.253.254.255\n"),
  IPV6_INVALID("1234::252.253. 254.255"),

  
  IPV6_INVALID("\0"),
  IPV6_INVALID("::1\0:2"),
  IPV6_INVALID("::1\0"),
  IPV6_INVALID("::1.2.3.4\0"),
  IPV6_INVALID("::1.2\02.3.4"),
};

class pkixnames_PresentedDNSIDMatchesReferenceDNSID
  : public ::testing::Test
  , public ::testing::WithParamInterface<PresentedMatchesReference>
{
};

TEST_P(pkixnames_PresentedDNSIDMatchesReferenceDNSID,
       PresentedDNSIDMatchesReferenceDNSID)
{
  const PresentedMatchesReference& param(GetParam());
  SCOPED_TRACE(param.presentedDNSID.c_str());
  SCOPED_TRACE(param.referenceDNSID.c_str());
  Input presented;
  ASSERT_EQ(Success, presented.Init(param.presentedDNSID.data(),
                                    param.presentedDNSID.length()));
  Input reference;
  ASSERT_EQ(Success, reference.Init(param.referenceDNSID.data(),
                                    param.referenceDNSID.length()));
  bool referenceIsValidDNSName = IsValidDNSName(reference);
  ASSERT_TRUE(referenceIsValidDNSName); 
  ASSERT_EQ(param.matches,
            PresentedDNSIDMatchesReferenceDNSID(presented, reference,
                                                referenceIsValidDNSName));
}

INSTANTIATE_TEST_CASE_P(pkixnames_PresentedDNSIDMatchesReferenceDNSID,
                        pkixnames_PresentedDNSIDMatchesReferenceDNSID,
                        testing::ValuesIn(DNSID_MATCH_PARAMS));

class pkixnames_Turkish_I_Comparison
  : public ::testing::Test
  , public ::testing::WithParamInterface<InputValidity>
{
};

TEST_P(pkixnames_Turkish_I_Comparison, PresentedDNSIDMatchesReferenceDNSID)
{
  
  

  const InputValidity& inputValidity(GetParam());
  SCOPED_TRACE(inputValidity.input.c_str());
  Input input;
  ASSERT_EQ(Success, input.Init(inputValidity.input.data(),
                                inputValidity.input.length()));
  bool isASCII = InputsAreEqual(LOWERCASE_I, input) ||
                 InputsAreEqual(UPPERCASE_I, input);
  ASSERT_EQ(isASCII, PresentedDNSIDMatchesReferenceDNSID(input, LOWERCASE_I,
                                                         true));
  ASSERT_EQ(isASCII, PresentedDNSIDMatchesReferenceDNSID(input, UPPERCASE_I,
                                                         true));
}

INSTANTIATE_TEST_CASE_P(pkixnames_Turkish_I_Comparison,
                        pkixnames_Turkish_I_Comparison,
                        testing::ValuesIn(DNSNAMES_VALIDITY_TURKISH_I));

class pkixnames_IsValidDNSName
  : public ::testing::Test
  , public ::testing::WithParamInterface<InputValidity>
{
};

TEST_P(pkixnames_IsValidDNSName, IsValidDNSName)
{
  const InputValidity& inputValidity(GetParam());
  SCOPED_TRACE(inputValidity.input.c_str());
  Input input;
  ASSERT_EQ(Success, input.Init(inputValidity.input.data(),
                                inputValidity.input.length()));
  ASSERT_EQ(inputValidity.isValid, IsValidDNSName(input));
}

INSTANTIATE_TEST_CASE_P(pkixnames_IsValidDNSName,
                        pkixnames_IsValidDNSName,
                        testing::ValuesIn(DNSNAMES_VALIDITY));
INSTANTIATE_TEST_CASE_P(pkixnames_IsValidDNSName_Turkish_I,
                        pkixnames_IsValidDNSName,
                        testing::ValuesIn(DNSNAMES_VALIDITY_TURKISH_I));

class pkixnames_ParseIPv4Address
  : public ::testing::Test
  , public ::testing::WithParamInterface<IPAddressParams<4>>
{
};

TEST_P(pkixnames_ParseIPv4Address, ParseIPv4Address)
{
  const IPAddressParams<4>& param(GetParam());
  SCOPED_TRACE(param.input.c_str());
  Input input;
  ASSERT_EQ(Success, input.Init(param.input.data(),
                                param.input.length()));
  uint8_t ipAddress[4];
  ASSERT_EQ(param.isValid, ParseIPv4Address(input, ipAddress));
  if (param.isValid) {
    for (size_t i = 0; i < sizeof(ipAddress); ++i) {
      ASSERT_EQ(param.expectedValueIfValid[i], ipAddress[i]);
    }
  }
}

INSTANTIATE_TEST_CASE_P(pkixnames_ParseIPv4Address,
                        pkixnames_ParseIPv4Address,
                        testing::ValuesIn(IPV4_ADDRESSES));

class pkixnames_ParseIPv6Address
  : public ::testing::Test
  , public ::testing::WithParamInterface<IPAddressParams<16>>
{
};

TEST_P(pkixnames_ParseIPv6Address, ParseIPv6Address)
{
  const IPAddressParams<16>& param(GetParam());
  SCOPED_TRACE(param.input.c_str());
  Input input;
  ASSERT_EQ(Success, input.Init(param.input.data(),
                                param.input.length()));
  uint8_t ipAddress[16];
  ASSERT_EQ(param.isValid, ParseIPv6Address(input, ipAddress));
  if (param.isValid) {
    for (size_t i = 0; i < sizeof(ipAddress); ++i) {
      ASSERT_EQ(param.expectedValueIfValid[i], ipAddress[i]);
    }
  }
}

INSTANTIATE_TEST_CASE_P(pkixnames_ParseIPv6Address,
                        pkixnames_ParseIPv6Address,
                        testing::ValuesIn(IPV6_ADDRESSES));
