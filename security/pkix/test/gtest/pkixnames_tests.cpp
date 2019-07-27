






















#include "pkixcheck.h"
#include "pkixder.h"
#include "pkixgtest.h"
#include "pkixutil.h"

namespace mozilla { namespace pkix {

Result MatchPresentedDNSIDWithReferenceDNSID(Input presentedDNSID,
                                             Input referenceDNSID,
                                              bool& matches);

bool IsValidReferenceDNSID(Input hostname);
bool IsValidPresentedDNSID(Input hostname);
bool ParseIPv4Address(Input hostname,  uint8_t (&out)[4]);
bool ParseIPv6Address(Input hostname,  uint8_t (&out)[16]);

} } 

using namespace mozilla::pkix;
using namespace mozilla::pkix::test;

struct PresentedMatchesReference
{
  ByteString presentedDNSID;
  ByteString referenceDNSID;
  Result expectedResult;
  bool expectedMatches; 
};

#define DNS_ID_MATCH(a, b) \
  { \
    ByteString(reinterpret_cast<const uint8_t*>(a), sizeof(a) - 1), \
    ByteString(reinterpret_cast<const uint8_t*>(b), sizeof(b) - 1), \
    Success, \
    true \
  }

#define DNS_ID_MISMATCH(a, b) \
  { \
    ByteString(reinterpret_cast<const uint8_t*>(a), sizeof(a) - 1), \
    ByteString(reinterpret_cast<const uint8_t*>(b), sizeof(b) - 1), \
    Success, \
    false \
  }

#define DNS_ID_BAD_DER(a, b) \
  { \
    ByteString(reinterpret_cast<const uint8_t*>(a), sizeof(a) - 1), \
    ByteString(reinterpret_cast<const uint8_t*>(b), sizeof(b) - 1), \
    Result::ERROR_BAD_DER, \
    false \
  }

static const PresentedMatchesReference DNSID_MATCH_PARAMS[] =
{
  DNS_ID_BAD_DER("", "a"),

  DNS_ID_MATCH("a", "a"),
  DNS_ID_MISMATCH("b", "a"),

  DNS_ID_MATCH("*.b.a", "c.b.a"),
  DNS_ID_MISMATCH("*.b.a", "b.a"),
  DNS_ID_MISMATCH("*.b.a", "b.a."),

  
  DNS_ID_MATCH("a_b", "a_b"),
  DNS_ID_MATCH("*.example.com", "uses_underscore.example.com"),
  DNS_ID_MATCH("*.uses_underscore.example.com", "a.uses_underscore.example.com"),

  
  DNS_ID_MATCH("_.example.com", "_.example.com"),
  DNS_ID_MATCH("*.example.com", "_.example.com"),
  DNS_ID_MATCH("_", "_"),
  DNS_ID_MATCH("___", "___"),
  DNS_ID_MATCH("example_", "example_"),
  DNS_ID_MATCH("_example", "_example"),
  DNS_ID_MATCH("*._._", "x._._"),

  
  
  
  DNS_ID_MATCH("_1", "_1"),
  DNS_ID_MATCH("example._1", "example._1"),
  DNS_ID_MATCH("example.1_", "example.1_"),

  
  DNS_ID_MATCH("d.c.b.a", "d.c.b.a"),
  DNS_ID_BAD_DER("d.*.b.a", "d.c.b.a"),
  DNS_ID_BAD_DER("d.c*.b.a", "d.c.b.a"),
  DNS_ID_BAD_DER("d.c*.b.a", "d.cc.b.a"),

  
  DNS_ID_MATCH("abcdefghijklmnopqrstuvwxyz", "ABCDEFGHIJKLMNOPQRSTUVWXYZ"),
  DNS_ID_MATCH("ABCDEFGHIJKLMNOPQRSTUVWXYZ", "abcdefghijklmnopqrstuvwxyz"),
  DNS_ID_MATCH("aBc", "Abc"),

  
  DNS_ID_MATCH("a1", "a1"),

  
  
  DNS_ID_MATCH("example", "example"),
  DNS_ID_BAD_DER("example.", "example."),
  DNS_ID_MATCH("example", "example."),
  DNS_ID_BAD_DER("example.", "example"),
  DNS_ID_MATCH("example.com", "example.com"),
  DNS_ID_BAD_DER("example.com.", "example.com."),
  DNS_ID_MATCH("example.com", "example.com."),
  DNS_ID_BAD_DER("example.com.", "example.com"),
  DNS_ID_BAD_DER("example.com..", "example.com."),
  DNS_ID_BAD_DER("example.com..", "example.com"),
  DNS_ID_BAD_DER("example.com...", "example.com."),

  
  DNS_ID_BAD_DER("x*.b.a", "xa.b.a"),
  DNS_ID_BAD_DER("x*.b.a", "xna.b.a"),
  DNS_ID_BAD_DER("x*.b.a", "xn-a.b.a"),
  DNS_ID_BAD_DER("x*.b.a", "xn--a.b.a"),
  DNS_ID_BAD_DER("xn*.b.a", "xn--a.b.a"),
  DNS_ID_BAD_DER("xn-*.b.a", "xn--a.b.a"),
  DNS_ID_BAD_DER("xn--*.b.a", "xn--a.b.a"),
  DNS_ID_BAD_DER("xn*.b.a", "xn--a.b.a"),
  DNS_ID_BAD_DER("xn-*.b.a", "xn--a.b.a"),
  DNS_ID_BAD_DER("xn--*.b.a", "xn--a.b.a"),
  DNS_ID_BAD_DER("xn---*.b.a", "xn--a.b.a"),

  
  DNS_ID_BAD_DER("c*.b.a", "c.b.a"),

  
  
  
  
  

  DNS_ID_MATCH("foo.com", "foo.com"),
  DNS_ID_MATCH("f", "f"),
  DNS_ID_MISMATCH("i", "h"),
  DNS_ID_MATCH("*.foo.com", "bar.foo.com"),
  DNS_ID_MATCH("*.test.fr", "www.test.fr"),
  DNS_ID_MATCH("*.test.FR", "wwW.tESt.fr"),
  DNS_ID_BAD_DER(".uk", "f.uk"),
  DNS_ID_BAD_DER("?.bar.foo.com", "w.bar.foo.com"),
  DNS_ID_BAD_DER("(www|ftp).foo.com", "www.foo.com"), 
  DNS_ID_BAD_DER("www.foo.com\0", "www.foo.com"),
  DNS_ID_BAD_DER("www.foo.com\0*.foo.com", "www.foo.com"),
  DNS_ID_MISMATCH("ww.house.example", "www.house.example"),
  DNS_ID_MISMATCH("www.test.org", "test.org"),
  DNS_ID_MISMATCH("*.test.org", "test.org"),
  DNS_ID_BAD_DER("*.org", "test.org"),
  DNS_ID_BAD_DER("w*.bar.foo.com", "w.bar.foo.com"),
  DNS_ID_BAD_DER("ww*ww.bar.foo.com", "www.bar.foo.com"),
  DNS_ID_BAD_DER("ww*ww.bar.foo.com", "wwww.bar.foo.com"),

  
  DNS_ID_BAD_DER("w*w.bar.foo.com", "wwww.bar.foo.com"),

  DNS_ID_BAD_DER("w*w.bar.foo.c0m", "wwww.bar.foo.com"),

  
  DNS_ID_BAD_DER("wa*.bar.foo.com", "WALLY.bar.foo.com"),

  
  
  DNS_ID_BAD_DER("*Ly.bar.foo.com", "wally.bar.foo.com"),

  
  
  
  

  DNS_ID_MISMATCH("*.test.de", "www.test.co.jp"),
  DNS_ID_BAD_DER("*.jp", "www.test.co.jp"),
  DNS_ID_MISMATCH("www.test.co.uk", "www.test.co.jp"),
  DNS_ID_BAD_DER("www.*.co.jp", "www.test.co.jp"),
  DNS_ID_MATCH("www.bar.foo.com", "www.bar.foo.com"),
  DNS_ID_MISMATCH("*.foo.com", "www.bar.foo.com"),
  DNS_ID_BAD_DER("*.*.foo.com", "www.bar.foo.com"),
  DNS_ID_BAD_DER("*.*.foo.com", "www.bar.foo.com"),

  
  
  

  DNS_ID_MATCH("www.bath.org", "www.bath.org"),

  
  
  
  
  

  
  DNS_ID_MATCH("xn--poema-9qae5a.com.br", "xn--poema-9qae5a.com.br"),
  DNS_ID_MATCH("*.xn--poema-9qae5a.com.br", "www.xn--poema-9qae5a.com.br"),
  DNS_ID_MISMATCH("*.xn--poema-9qae5a.com.br", "xn--poema-9qae5a.com.br"),
  DNS_ID_BAD_DER("xn--poema-*.com.br", "xn--poema-9qae5a.com.br"),
  DNS_ID_BAD_DER("xn--*-9qae5a.com.br", "xn--poema-9qae5a.com.br"),
  DNS_ID_BAD_DER("*--poema-9qae5a.com.br", "xn--poema-9qae5a.com.br"),

  
  
  
  
  DNS_ID_MATCH("*.example.com", "foo.example.com"),
  DNS_ID_MISMATCH("*.example.com", "bar.foo.example.com"),
  DNS_ID_MISMATCH("*.example.com", "example.com"),
  
  
  
  
  DNS_ID_BAD_DER("baz*.example.net", "baz1.example.net"),

  
  
  DNS_ID_BAD_DER("*baz.example.net", "foobaz.example.net"),
  DNS_ID_BAD_DER("b*z.example.net", "buzz.example.net"),

  
  
  
  
  DNS_ID_MATCH("*.test.example", "www.test.example"),
  DNS_ID_MATCH("*.example.co.uk", "test.example.co.uk"),
  DNS_ID_BAD_DER("*.exmaple", "test.example"),

  
  
  
  DNS_ID_MATCH("*.co.uk", "example.co.uk"),

  DNS_ID_BAD_DER("*.com", "foo.com"),
  DNS_ID_BAD_DER("*.us", "foo.us"),
  DNS_ID_BAD_DER("*", "foo"),

  
  DNS_ID_MATCH("*.xn--poema-9qae5a.com.br", "www.xn--poema-9qae5a.com.br"),
  DNS_ID_MATCH("*.example.xn--mgbaam7a8h", "test.example.xn--mgbaam7a8h"),

  
  
  DNS_ID_MATCH("*.com.br", "xn--poema-9qae5a.com.br"),

  DNS_ID_BAD_DER("*.xn--mgbaam7a8h", "example.xn--mgbaam7a8h"),
  
  
  
  DNS_ID_MATCH("*.appspot.com", "www.appspot.com"),
  DNS_ID_MATCH("*.s3.amazonaws.com", "foo.s3.amazonaws.com"),

  
  DNS_ID_BAD_DER("*.*.com", "foo.example.com"),
  DNS_ID_BAD_DER("*.bar.*.com", "foo.bar.example.com"),

  
  
  
  
  
  DNS_ID_BAD_DER("foo.com.", "foo.com"),
  DNS_ID_MATCH("foo.com", "foo.com."),
  DNS_ID_BAD_DER("foo.com.", "foo.com."),
  DNS_ID_BAD_DER("f.", "f"),
  DNS_ID_MATCH("f", "f."),
  DNS_ID_BAD_DER("f.", "f."),
  DNS_ID_BAD_DER("*.bar.foo.com.", "www-3.bar.foo.com"),
  DNS_ID_MATCH("*.bar.foo.com", "www-3.bar.foo.com."),
  DNS_ID_BAD_DER("*.bar.foo.com.", "www-3.bar.foo.com."),

  
  
  

  DNS_ID_BAD_DER("*.com.", "example.com"),
  DNS_ID_BAD_DER("*.com", "example.com."),
  DNS_ID_BAD_DER("*.com.", "example.com."),
  DNS_ID_BAD_DER("*.", "foo."),
  DNS_ID_BAD_DER("*.", "foo"),

  
  
  DNS_ID_MATCH("*.co.uk", "foo.co.uk"),
  DNS_ID_MATCH("*.co.uk", "foo.co.uk."),
  DNS_ID_BAD_DER("*.co.uk.", "foo.co.uk"),
  DNS_ID_BAD_DER("*.co.uk.", "foo.co.uk."),

  DNS_ID_MISMATCH("*.example.com", "localhost"),
  DNS_ID_MISMATCH("*.example.com", "localhost."),
  
};

struct InputValidity
{
  ByteString input;
  bool isValidReferenceID;
  bool isValidPresentedID;
};



#define I(str, validReferenceID, validPresentedID) \
  { \
    ByteString(reinterpret_cast<const uint8_t*>(str), sizeof(str) - 1), \
    validReferenceID, \
    validPresentedID, \
  }

static const InputValidity DNSNAMES_VALIDITY[] =
{
  I("a", true, true),
  I("a.b", true, true),
  I("a.b.c", true, true),
  I("a.b.c.d", true, true),

  
  I("", false, false),
  I(".", false, false),
  I("a", true, true),
  I(".a", false, false),
  I(".a.b", false, false),
  I("..a", false, false),
  I("a..b", false, false),
  I("a...b", false, false),
  I("a..b.c", false, false),
  I("a.b..c", false, false),
  I(".a.b.c.", false, false),

  
  I("a.", true, false),
  I("a.b.", true, false),
  I("a.b.c.", true, false),

  
  I("a..", false, false),
  I("a.b..", false, false),
  I("a.b.c..", false, false),
  I("a...", false, false),

  
  I("xn--", false, false),
  I("xn--.", false, false),
  I("xn--.a", false, false),
  I("a.xn--", false, false),
  I("a.xn--.", false, false),
  I("a.xn--.b", false, false),
  I("a.xn--.b", false, false),
  I("a.xn--\0.b", false, false),
  I("a.xn--a.b", true, true),
  I("xn--a", true, true),
  I("a.xn--a", true, true),
  I("a.xn--a.a", true, true),
  I("\xc4\x95.com", false, false), 
  I("xn--jea.com", true, true), 
  I("xn--\xc4\x95.com", false, false), 

  
  I("xn--google.com", true, true), 
  I("xn--citibank.com", true, true), 
  I("xn--cnn.com", true, true), 
  I("a.xn--cnn", true, true), 
  I("a.xn--cnn.com", true, true), 

  I("1.2.3.4", false, false), 
  I("1::2", false, false), 

  
  I(" ", false, false),
  I(" a", false, false),
  I("a ", false, false),
  I("a b", false, false),
  I("a.b 1", false, false),
  I("a\t", false, false),

  
  I("\0", false, false),
  I("a\0", false, false),
  I("example.org\0.example.com", false, false), 
  I("\0a", false, false),
  I("xn--\0", false, false),

  
  I("a.b.c.d.e.f.g.h.i.j.k.l.m.n.o.p.q.r.s.t.u.v.w.x.y.z", true, true),
  I("A.B.C.D.E.F.G.H.I.J.K.L.M.N.O.P.Q.R.S.T.U.V.W.X.Y.Z", true, true),
  I("0.1.2.3.4.5.6.7.8.9.a", true, true), 
  I("a-b", true, true), 

  
  I("a_b", true, true),
  
  I("_", true, true),
  I("a_", true, true),
  I("_a", true, true),
  I("_1", true, true),
  I("1_", true, true),
  I("___", true, true),

  
  I("!", false, false),
  I("!a", false, false),
  I("a!", false, false),
  I("a!b", false, false),
  I("a.!", false, false),
  I("a.a!", false, false),
  I("a.!a", false, false),
  I("a.a!a", false, false),
  I("a.!a.a", false, false),
  I("a.a!.a", false, false),
  I("a.a!a.a", false, false),

  
  I("a!", false, false),
  I("a@", false, false),
  I("a#", false, false),
  I("a$", false, false),
  I("a%", false, false),
  I("a^", false, false),
  I("a&", false, false),
  I("a*", false, false),
  I("a(", false, false),
  I("a)", false, false),

  
  I("1", false, false),
  I("a.1", false, false),

  
  I("1.a", true, true),
  I("1.2.a", true, true),
  I("1.2.3.a", true, true),

  
  I("1a", true, true),
  I("1.1a", true, true),
  I("1-1", true, true),
  I("a.1-1", true, true),
  I("a.1-a", true, true),

  
  I("-", false, false),
  I("-1", false, false),

  
  I("1-", false, false),
  I("1-.a", false, false),
  I("a-", false, false),
  I("a-.a", false, false),
  I("a.1-.a", false, false),
  I("a.a-.a", false, false),

  
  I("a-b", true, true),
  I("1-2", true, true),
  I("a.a-1", true, true),

  
  I("a--1", true, true),
  I("1---a", true, true),
  I("a-----------------b", true, true),

  
  
  
  I("*.a", false, false),
  I("a*", false, false),
  I("a*.", false, false),
  I("a*.a", false, false),
  I("a*.a.", false, false),
  I("*.a.b", false, true),
  I("*.a.b.", false, false),
  I("a*.b.c", false, false),
  I("*.a.b.c", false, true),
  I("a*.b.c.d", false, false),

  
  I("a**.b.c", false, false),
  I("a*b*.c.d", false, false),
  I("a*.b*.c", false, false),

  
  I("a.*", false, false),
  I("a.*.b", false, false),
  I("a.b.*", false, false),
  I("a.b*.c", false, false),
  I("*.b*.c", false, false),
  I(".*.a.b", false, false),
  I(".a*.b.c", false, false),

  
  I("*a.b.c", false, false),
  I("a*b.c.d", false, false),

  
  I("x*.a.b", false, false),
  I("xn*.a.b", false, false),
  I("xn-*.a.b", false, false),
  I("xn--*.a.b", false, false),
  I("xn--w*.a.b", false, false),

  
  
  I("(PRIVATE).foo", false, false),

  
  I("1234567890" "1234567890" "1234567890"
    "1234567890" "1234567890" "1234567890" "abc", true, true),
  I("1234567890" "1234567890" "1234567890"
    "1234567890" "1234567890" "1234567890" "abcd", false, false),

  
  I("1234567890" "1234567890" "1234567890" "1234567890" "1234567890" "."
    "1234567890" "1234567890" "1234567890" "1234567890" "1234567890" "."
    "1234567890" "1234567890" "1234567890" "1234567890" "1234567890" "."
    "1234567890" "1234567890" "1234567890" "1234567890" "1234567890" "."
    "1234567890" "1234567890" "1234567890" "1234567890" "12345678" "a",
    true, true),
  I("1234567890" "1234567890" "1234567890" "1234567890" "1234567890" "."
    "1234567890" "1234567890" "1234567890" "1234567890" "1234567890" "."
    "1234567890" "1234567890" "1234567890" "1234567890" "1234567890" "."
    "1234567890" "1234567890" "1234567890" "1234567890" "1234567890" "."
    "1234567890" "1234567890" "1234567890" "1234567890" "123456789" "a",
    false, false),
};

static const InputValidity DNSNAMES_VALIDITY_TURKISH_I[] =
{
  
  
  
  I("I", true, true), 
  I("i", true, true), 
  I("\xC4\xB0", false, false), 
  I("\xC4\xB1", false, false), 
  I("xn--i-9bb", true, true), 
  I("xn--cfa", true, true), 
  I("xn--\xC4\xB0", false, false), 
  I("xn--\xC4\xB1", false, false), 
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

class pkixnames_MatchPresentedDNSIDWithReferenceDNSID
  : public ::testing::Test
  , public ::testing::WithParamInterface<PresentedMatchesReference>
{
};

TEST_P(pkixnames_MatchPresentedDNSIDWithReferenceDNSID,
       MatchPresentedDNSIDWithReferenceDNSID)
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

  
  ASSERT_TRUE(IsValidReferenceDNSID(reference));

  bool matches;
  ASSERT_EQ(param.expectedResult,
            MatchPresentedDNSIDWithReferenceDNSID(presented, reference,
                                                  matches));
  if (param.expectedResult == Success) {
    ASSERT_EQ(param.expectedMatches, matches);
  }
}

INSTANTIATE_TEST_CASE_P(pkixnames_MatchPresentedDNSIDWithReferenceDNSID,
                        pkixnames_MatchPresentedDNSIDWithReferenceDNSID,
                        testing::ValuesIn(DNSID_MATCH_PARAMS));

class pkixnames_Turkish_I_Comparison
  : public ::testing::Test
  , public ::testing::WithParamInterface<InputValidity>
{
};

TEST_P(pkixnames_Turkish_I_Comparison, MatchPresentedDNSIDWithReferenceDNSID)
{
  
  

  const InputValidity& inputValidity(GetParam());
  SCOPED_TRACE(inputValidity.input.c_str());
  Input input;
  ASSERT_EQ(Success, input.Init(inputValidity.input.data(),
                                inputValidity.input.length()));

  bool isASCII = InputsAreEqual(LOWERCASE_I, input) ||
                 InputsAreEqual(UPPERCASE_I, input);
  {
    bool matches;
    ASSERT_EQ(inputValidity.isValidPresentedID ? Success
                                               : Result::ERROR_BAD_DER,
              MatchPresentedDNSIDWithReferenceDNSID(input, LOWERCASE_I,
                                                    matches));
    if (inputValidity.isValidPresentedID) {
      ASSERT_EQ(isASCII, matches);
    }
  }
  {
    bool matches;
    ASSERT_EQ(inputValidity.isValidPresentedID ? Success
                                               : Result::ERROR_BAD_DER,
              MatchPresentedDNSIDWithReferenceDNSID(input, UPPERCASE_I,
                                                    matches));
    if (inputValidity.isValidPresentedID) {
      ASSERT_EQ(isASCII, matches);
    }
  }
}

INSTANTIATE_TEST_CASE_P(pkixnames_Turkish_I_Comparison,
                        pkixnames_Turkish_I_Comparison,
                        testing::ValuesIn(DNSNAMES_VALIDITY_TURKISH_I));

class pkixnames_IsValidReferenceDNSID
  : public ::testing::Test
  , public ::testing::WithParamInterface<InputValidity>
{
};

TEST_P(pkixnames_IsValidReferenceDNSID, IsValidReferenceDNSID)
{
  const InputValidity& inputValidity(GetParam());
  SCOPED_TRACE(inputValidity.input.c_str());
  Input input;
  ASSERT_EQ(Success, input.Init(inputValidity.input.data(),
                                inputValidity.input.length()));
  ASSERT_EQ(inputValidity.isValidReferenceID, IsValidReferenceDNSID(input));
  ASSERT_EQ(inputValidity.isValidPresentedID, IsValidPresentedDNSID(input));
}

INSTANTIATE_TEST_CASE_P(pkixnames_IsValidReferenceDNSID,
                        pkixnames_IsValidReferenceDNSID,
                        testing::ValuesIn(DNSNAMES_VALIDITY));
INSTANTIATE_TEST_CASE_P(pkixnames_IsValidReferenceDNSID_Turkish_I,
                        pkixnames_IsValidReferenceDNSID,
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






static const ByteString
  NO_SAN(reinterpret_cast<const uint8_t*>("I'm a bad, bad, certificate"));

struct CheckCertHostnameParams
{
  ByteString hostname;
  ByteString subject;
  ByteString subjectAltName;
  Result result;
};

class pkixnames_CheckCertHostname
  : public ::testing::Test
  , public ::testing::WithParamInterface<CheckCertHostnameParams>
{
};

#define WITH_SAN(r, ps, psan, result) \
  { \
    ByteString(reinterpret_cast<const uint8_t*>(r), sizeof(r) - 1), \
    ps, \
    psan, \
    result \
  }

#define WITHOUT_SAN(r, ps, result) \
  { \
    ByteString(reinterpret_cast<const uint8_t*>(r), sizeof(r) - 1), \
    ps, \
    NO_SAN, \
    result \
  }

static const uint8_t example_com[] = {
  'e', 'x', 'a', 'm', 'p', 'l', 'e', '.', 'c', 'o', 'm'
};



static const uint8_t ipv4_addr_bytes[] = {
  1, 2, 3, 4
};
static const uint8_t ipv4_addr_bytes_as_str[] = "\x01\x02\x03\x04";
static const uint8_t ipv4_addr_str[] = "1.2.3.4";
static const uint8_t ipv4_addr_bytes_FFFFFFFF[8] = {
  1, 2, 3, 4, 0xff, 0xff, 0xff, 0xff
};

static const uint8_t ipv4_compatible_ipv6_addr_bytes[] = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  1, 2, 3, 4
};
static const uint8_t ipv4_compatible_ipv6_addr_str[] = "::1.2.3.4";

static const uint8_t ipv4_mapped_ipv6_addr_bytes[] = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0xFF, 0xFF,
  1, 2, 3, 4
};
static const uint8_t ipv4_mapped_ipv6_addr_str[] = "::FFFF:1.2.3.4";

static const uint8_t ipv6_addr_bytes[] = {
  0x11, 0x22, 0x33, 0x44,
  0x55, 0x66, 0x77, 0x88,
  0x99, 0xaa, 0xbb, 0xcc,
  0xdd, 0xee, 0xff, 0x11
};
static const uint8_t ipv6_addr_bytes_as_str[] =
  "\x11\x22\x33\x44"
  "\x55\x66\x77\x88"
  "\x99\xaa\xbb\xcc"
  "\xdd\xee\xff\x11";

static const uint8_t ipv6_addr_str[] =
  "1122:3344:5566:7788:99aa:bbcc:ddee:ff11";

static const uint8_t ipv6_other_addr_bytes[] = {
  0xff, 0xee, 0xdd, 0xcc,
  0xbb, 0xaa, 0x99, 0x88,
  0x77, 0x66, 0x55, 0x44,
  0x33, 0x22, 0x11, 0x00,
};

static const uint8_t ipv4_other_addr_bytes[] = {
  5, 6, 7, 8
};
static const uint8_t ipv4_other_addr_bytes_FFFFFFFF[] = {
  5, 6, 7, 8, 0xff, 0xff, 0xff, 0xff
};

static const uint8_t ipv4_addr_00000000_bytes[] = {
  0, 0, 0, 0
};
static const uint8_t ipv4_addr_FFFFFFFF_bytes[] = {
  0, 0, 0, 0
};

static const uint8_t ipv4_constraint_all_zeros_bytes[] = {
  0, 0, 0, 0, 0, 0, 0, 0
};

static const uint8_t ipv6_addr_all_zeros_bytes[] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
};

static const uint8_t ipv6_constraint_all_zeros_bytes[] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};

static const uint8_t ipv4_constraint_CIDR_16_bytes[] = {
  1, 2, 0, 0, 0xff, 0xff, 0, 0
};
static const uint8_t ipv4_constraint_CIDR_17_bytes[] = {
  1, 2, 0, 0, 0xff, 0xff, 0x80, 0
};


static const uint8_t ipv4_constraint_CIDR_16_bad_addr_bytes[] = {
  1, 2, 3, 0, 0xff, 0xff, 0, 0
};



static const uint8_t ipv4_constraint_bad_mask_bytes[] = {
  1, 2, 3, 0, 0xff, 0, 0xff, 0
};

static const uint8_t ipv6_constraint_CIDR_16_bytes[] = {
  0x11, 0x22, 0, 0, 0, 0, 0, 0,
     0,    0, 0, 0, 0, 0, 0, 0,
  0xff, 0xff, 0, 0, 0, 0, 0, 0,
     0,    0, 0, 0, 0, 0, 0, 0
};


static const uint8_t ipv6_constraint_CIDR_16_bad_addr_bytes[] = {
  0x11, 0x22, 0x33, 0x44, 0, 0, 0, 0,
     0,    0,    0,    0, 0, 0, 0, 0,
  0xff, 0xff,    0,    0, 0, 0, 0, 0,
     0,    0,    0,    0, 0, 0, 0, 0
};



static const uint8_t ipv6_constraint_bad_mask_bytes[] = {
  0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0, 0,
     0,    0,    0,    0,    0,    0, 0, 0,
  0xff, 0xff,    0,    0, 0xff, 0xff, 0, 0,
     0,    0,    0,    0,    0,    0, 0, 0,
};

static const uint8_t ipv4_addr_truncated_bytes[] = {
  1, 2, 3
};
static const uint8_t ipv4_addr_overlong_bytes[] = {
  1, 2, 3, 4, 5
};
static const uint8_t ipv4_constraint_truncated_bytes[] = {
  0, 0, 0, 0,
  0, 0, 0,
};
static const uint8_t ipv4_constraint_overlong_bytes[] = {
  0, 0, 0, 0,
  0, 0, 0, 0, 0
};

static const uint8_t ipv6_addr_truncated_bytes[] = {
  0x11, 0x22, 0x33, 0x44,
  0x55, 0x66, 0x77, 0x88,
  0x99, 0xaa, 0xbb, 0xcc,
  0xdd, 0xee, 0xff
};
static const uint8_t ipv6_addr_overlong_bytes[] = {
  0x11, 0x22, 0x33, 0x44,
  0x55, 0x66, 0x77, 0x88,
  0x99, 0xaa, 0xbb, 0xcc,
  0xdd, 0xee, 0xff, 0x11, 0x00
};
static const uint8_t ipv6_constraint_truncated_bytes[] = {
  0x11, 0x22, 0, 0, 0, 0, 0, 0,
     0,    0, 0, 0, 0, 0, 0, 0,
  0xff, 0xff, 0, 0, 0, 0, 0, 0,
     0,    0, 0, 0, 0, 0, 0
};
static const uint8_t ipv6_constraint_overlong_bytes[] = {
  0x11, 0x22, 0, 0, 0, 0, 0, 0,
     0,    0, 0, 0, 0, 0, 0, 0,
  0xff, 0xff, 0, 0, 0, 0, 0, 0,
     0,    0, 0, 0, 0, 0, 0, 0, 0
};










static const CheckCertHostnameParams CHECK_CERT_HOSTNAME_PARAMS[] =
{
  
  
  
  WITHOUT_SAN("foo.example.com", RDN(CN("*.example.com", der::PrintableString)),
              Success),
  WITHOUT_SAN("foo.example.com", RDN(CN("*.example.com", der::UTF8String)),
              Success),

  
  
  
  
  
  
  
  
  WITHOUT_SAN("foo.example.com", RDN(CN("*.example.com", der::TeletexString)),
              Success),
  
  
  WITHOUT_SAN("foo.example.com",
              RDN(CN("\x1B(B*.example.com", der::TeletexString)),
              Result::ERROR_BAD_CERT_DOMAIN),
  WITHOUT_SAN("foo.example.com",
              RDN(CN("*.example\x1B(B.com", der::TeletexString)),
              Result::ERROR_BAD_CERT_DOMAIN),
  WITHOUT_SAN("foo.example.com",
              RDN(CN("*.example.com\x1B(B", der::TeletexString)),
              Result::ERROR_BAD_CERT_DOMAIN),
  
  
  WITHOUT_SAN("foo.example.com",
              RDN(CN("\x1B$B*.example.com", der::TeletexString)),
              Result::ERROR_BAD_CERT_DOMAIN),
  WITHOUT_SAN("foo.example.com",
              RDN(CN("*.example.com\x1B$B", der::TeletexString)),
              Result::ERROR_BAD_CERT_DOMAIN),

  
  WITH_SAN("a", RDN(CN("a")), DNSName("a"), Success),
  
  WITH_SAN("b", RDN(CN("a")), DNSName("b"), Success),
  
  WITH_SAN("a", RDN(CN("a")), DNSName("b"), Result::ERROR_BAD_CERT_DOMAIN),
  
  WITH_SAN("a", RDN(CN("a")), DNSName("!"), Result::ERROR_BAD_DER),
  
  WITH_SAN("a", RDN(CN("a")), IPAddress(ipv4_addr_bytes),
           Result::ERROR_BAD_CERT_DOMAIN),
  
  WITH_SAN("a", RDN(CN("a")), IPAddress(example_com),
           Result::ERROR_BAD_CERT_DOMAIN),
  
  
  WITH_SAN("a", RDN(CN("a")), RFC822Name("foo@example.com"), Success),
  
  WITHOUT_SAN("a", RDN(CN("a")), Success),
  
  WITHOUT_SAN("a", RDN(CN("b")), Result::ERROR_BAD_CERT_DOMAIN),

  
  WITH_SAN("a", RDN(CN("foo")), DNSName("a") + DNSName("b"), Success),
  
  WITH_SAN("b", RDN(CN("foo")), DNSName("a") + DNSName("b"), Success),
  
  WITH_SAN("b", RDN(CN("foo")),
           DNSName("a") + DNSName("b") + DNSName("c"), Success),
  
  WITH_SAN("b", RDN(CN("foo")),
           IPAddress(ipv4_addr_bytes) + DNSName("b"), Success),
  
  WITH_SAN("a", RDN(CN("foo")),
           DNSName("a") + IPAddress(ipv4_addr_bytes), Success),
  
  WITH_SAN("b", RDN(CN("foo")),
           RFC822Name("foo@example.com") + DNSName("b") +
                                           IPAddress(ipv4_addr_bytes),
           Success),
  
  WITH_SAN("a", RDN(CN("foo")), DNSName("a") + DNSName("a"), Success),
  
  WITH_SAN("b", RDN(CN("foo")), DNSName("!") + DNSName("b"),
           Result::ERROR_BAD_DER),

  
  
  
  
  
  WITH_SAN("a", RDN(CN("a")), ByteString(), Success),
  WITH_SAN("a", RDN(CN("b")), ByteString(), Result::ERROR_BAD_CERT_DOMAIN),

  
  
  
  
  
  
  WITH_SAN("a", ByteString(), DNSName("a"), Success),
  
  WITHOUT_SAN("a", ByteString(), Result::ERROR_BAD_CERT_DOMAIN),

  
  WITHOUT_SAN("a", RDN(CN("a") + CN("a")), Success),
  
  WITHOUT_SAN("a", RDN(CN("a") + CN("b")),
              Result::ERROR_BAD_CERT_DOMAIN),
  
  WITHOUT_SAN("b", RDN(CN("a") + CN("b")), Success),
  
  WITHOUT_SAN("a", RDN(CN("a") + CN("Not a DNSName")),
              Result::ERROR_BAD_CERT_DOMAIN),
  
  WITHOUT_SAN("b", RDN(CN("Not a DNSName") + CN("b")), Success),

  
  WITHOUT_SAN("a", RDN(CN("a")) + RDN(CN("a")), Success),
  
  WITHOUT_SAN("a", RDN(CN("a")) + RDN(CN("b")),
              Result::ERROR_BAD_CERT_DOMAIN),
  
  WITHOUT_SAN("b", RDN(CN("a")) + RDN(CN("b")), Success),
  
  WITHOUT_SAN("a", RDN(CN("a")) + RDN(CN("Not a DNSName")),
              Result::ERROR_BAD_CERT_DOMAIN),
  
  WITHOUT_SAN("b", RDN(CN("Not a DNSName")) + RDN(CN("b")), Success),

  
  WITHOUT_SAN("a", RDN(CN("a") + OU("b")), Success),
  
  WITHOUT_SAN("b", RDN(CN("a") + OU("b")),
              Result::ERROR_BAD_CERT_DOMAIN),
  
  WITHOUT_SAN("b", RDN(OU("a") + CN("b")), Success),
  
  WITHOUT_SAN("a", RDN(OU("a") + CN("b")),
              Result::ERROR_BAD_CERT_DOMAIN),

  
  WITHOUT_SAN("a", RDN(CN("a")) + RDN(OU("b")), Success),
  
  WITHOUT_SAN("b", RDN(CN("a")) + RDN(OU("b")), Result::ERROR_BAD_CERT_DOMAIN),
  
  WITHOUT_SAN("b", RDN(OU("a")) + RDN(CN("b")), Success),
  
  WITHOUT_SAN("a", RDN(OU("a")) + RDN(CN("b")), Result::ERROR_BAD_CERT_DOMAIN),

  
  WITHOUT_SAN("b", RDN(OU("a") + CN("b") + OU("c")), Success),
  
  WITHOUT_SAN("b", RDN(OU("a")) + RDN(CN("b")) + RDN(OU("c")), Success),

  
  WITHOUT_SAN("example.com", RDN(CN("")), Result::ERROR_BAD_CERT_DOMAIN),

  WITHOUT_SAN("uses_underscore.example.com", RDN(CN("*.example.com")), Success),
  WITHOUT_SAN("a.uses_underscore.example.com",
              RDN(CN("*.uses_underscore.example.com")), Success),
  WITH_SAN("uses_underscore.example.com", RDN(CN("foo")),
           DNSName("*.example.com"), Success),
  WITH_SAN("a.uses_underscore.example.com", RDN(CN("foo")),
           DNSName("*.uses_underscore.example.com"), Success),

  
  WITH_SAN("example.com", RDN(CN("foo")), IPAddress(example_com),
           Result::ERROR_BAD_CERT_DOMAIN),

  
  
  
  WITH_SAN("example.org", RDN(CN("foo")),
           IPAddress(example_com) + DNSName("example.org"), Success),

  WITH_SAN("example.com", RDN(CN("foo")),
           DNSName("!") + DNSName("example.com"), Result::ERROR_BAD_DER),

  
  WITH_SAN(ipv4_addr_str, RDN(CN("foo")), IPAddress(ipv4_addr_bytes),
           Success),
  
  WITHOUT_SAN(ipv4_addr_str, RDN(CN(ipv4_addr_str)), Success),
  
  
  WITH_SAN(ipv4_addr_str, RDN(CN(ipv4_addr_str)),
           DNSName("example.com"), Result::ERROR_BAD_CERT_DOMAIN),
  
  
  WITH_SAN(ipv4_addr_str, RDN(CN(ipv4_addr_str)),
           IPAddress(ipv6_addr_bytes), Result::ERROR_BAD_CERT_DOMAIN),
  
  
  WITH_SAN(ipv4_addr_str, RDN(CN(ipv4_addr_str)),
           RFC822Name("foo@example.com"), Success),
  
  
  WITH_SAN(ipv4_addr_str, RDN(CN(ipv4_addr_str)),
           IPAddress(example_com), Result::ERROR_BAD_CERT_DOMAIN),
  
  
  WITH_SAN(ipv4_addr_str, RDN(CN(ipv4_addr_str)),
           DNSName("!"), Result::ERROR_BAD_CERT_DOMAIN),

  
  
  WITHOUT_SAN(ipv6_addr_str, RDN(CN(ipv6_addr_str)),
              Result::ERROR_BAD_CERT_DOMAIN),
  WITH_SAN(ipv6_addr_str, RDN(CN(ipv6_addr_str)),
           DNSName("example.com"), Result::ERROR_BAD_CERT_DOMAIN),
  WITH_SAN(ipv6_addr_str, RDN(CN(ipv6_addr_str)),
                          IPAddress(ipv6_addr_bytes), Success),
  WITH_SAN(ipv6_addr_str, RDN(CN("foo")), IPAddress(ipv6_addr_bytes),
           Success),

  
  
  WITHOUT_SAN(ipv4_addr_str, RDN(CN(ipv4_addr_bytes_as_str)),
              Result::ERROR_BAD_CERT_DOMAIN),
  WITHOUT_SAN(ipv6_addr_str, RDN(CN(ipv6_addr_bytes_as_str)),
              Result::ERROR_BAD_CERT_DOMAIN),

  
  WITH_SAN(ipv4_addr_str, RDN(CN("foo")),
           DNSName(ipv4_addr_bytes_as_str), Result::ERROR_BAD_CERT_DOMAIN),
  WITH_SAN(ipv4_addr_str, RDN(CN("foo")), DNSName(ipv4_addr_str),
           Result::ERROR_BAD_CERT_DOMAIN),
  WITH_SAN(ipv6_addr_str, RDN(CN("foo")),
           DNSName(ipv6_addr_bytes_as_str), Result::ERROR_BAD_CERT_DOMAIN),
  WITH_SAN(ipv6_addr_str, RDN(CN("foo")), DNSName(ipv6_addr_str),
           Result::ERROR_BAD_CERT_DOMAIN),

  
  
  WITH_SAN(ipv4_addr_str, RDN(CN("foo")),
           IPAddress(ipv4_compatible_ipv6_addr_bytes),
           Result::ERROR_BAD_CERT_DOMAIN),
  
  
  WITH_SAN(ipv4_addr_str, RDN(CN("foo")),
           IPAddress(ipv4_mapped_ipv6_addr_bytes),
           Result::ERROR_BAD_CERT_DOMAIN),
  
  
  WITH_SAN(ipv4_compatible_ipv6_addr_str, RDN(CN("foo")),
           IPAddress(ipv4_addr_bytes), Result::ERROR_BAD_CERT_DOMAIN),
  
  
  WITH_SAN(ipv4_mapped_ipv6_addr_str, RDN(CN("foo")),
           IPAddress(ipv4_addr_bytes),
           Result::ERROR_BAD_CERT_DOMAIN),

  
  
  
  WITH_SAN("example.com", ByteString(),
           
           TLV((2 << 6) | (1 << 5) | 0, ByteString()) + DNSName("example.com"),
           Success),
  WITH_SAN("example.com", ByteString(),
           TLV((2 << 6) | (1 << 5) | 0, ByteString()),
           Result::ERROR_BAD_CERT_DOMAIN),
};

ByteString
CreateCert(const ByteString& subject, const ByteString& subjectAltName)
{
  ByteString serialNumber(CreateEncodedSerialNumber(1));
  EXPECT_FALSE(ENCODING_FAILED(serialNumber));

  ByteString issuerDER(Name(RDN(CN("issuer"))));
  EXPECT_FALSE(ENCODING_FAILED(issuerDER));

  ByteString extensions[2];
  if (subjectAltName != NO_SAN) {
    extensions[0] = CreateEncodedSubjectAltName(subjectAltName);
    EXPECT_FALSE(ENCODING_FAILED(extensions[0]));
  }

  ScopedTestKeyPair keyPair(CloneReusedKeyPair());
  return CreateEncodedCertificate(
                    v3, sha256WithRSAEncryption(), serialNumber, issuerDER,
                    oneDayBeforeNow, oneDayAfterNow, Name(subject), *keyPair,
                    extensions, *keyPair, sha256WithRSAEncryption());
}

TEST_P(pkixnames_CheckCertHostname, CheckCertHostname)
{
  const CheckCertHostnameParams& param(GetParam());

  ByteString cert(CreateCert(param.subject, param.subjectAltName));
  ASSERT_FALSE(ENCODING_FAILED(cert));
  Input certInput;
  ASSERT_EQ(Success, certInput.Init(cert.data(), cert.length()));

  Input hostnameInput;
  ASSERT_EQ(Success, hostnameInput.Init(param.hostname.data(),
                                        param.hostname.length()));

  ASSERT_EQ(param.result, CheckCertHostname(certInput, hostnameInput));
}

INSTANTIATE_TEST_CASE_P(pkixnames_CheckCertHostname,
                        pkixnames_CheckCertHostname,
                        testing::ValuesIn(CHECK_CERT_HOSTNAME_PARAMS));

TEST_F(pkixnames_CheckCertHostname, SANWithoutSequence)
{
  
  
  

  ByteString serialNumber(CreateEncodedSerialNumber(1));
  EXPECT_FALSE(ENCODING_FAILED(serialNumber));

  ByteString extensions[2];
  extensions[0] = CreateEncodedEmptySubjectAltName();
  ASSERT_FALSE(ENCODING_FAILED(extensions[0]));

  ScopedTestKeyPair keyPair(CloneReusedKeyPair());
  ByteString certDER(CreateEncodedCertificate(
                       v3, sha256WithRSAEncryption(), serialNumber,
                       Name(RDN(CN("issuer"))), oneDayBeforeNow, oneDayAfterNow,
                       Name(RDN(CN("a"))), *keyPair, extensions,
                       *keyPair, sha256WithRSAEncryption()));
  ASSERT_FALSE(ENCODING_FAILED(certDER));
  Input certInput;
  ASSERT_EQ(Success, certInput.Init(certDER.data(), certDER.length()));

  static const uint8_t a[] = { 'a' };
  ASSERT_EQ(Result::ERROR_EXTENSION_VALUE_INVALID,
            CheckCertHostname(certInput, Input(a)));
}

class pkixnames_CheckCertHostname_PresentedMatchesReference
  : public ::testing::Test
  , public ::testing::WithParamInterface<PresentedMatchesReference>
{
};

TEST_P(pkixnames_CheckCertHostname_PresentedMatchesReference, CN_NoSAN)
{
  
  

  const PresentedMatchesReference& param(GetParam());

  ByteString cert(CreateCert(RDN(CN(param.presentedDNSID)), NO_SAN));
  ASSERT_FALSE(ENCODING_FAILED(cert));
  Input certInput;
  ASSERT_EQ(Success, certInput.Init(cert.data(), cert.length()));

  Input hostnameInput;
  ASSERT_EQ(Success, hostnameInput.Init(param.referenceDNSID.data(),
                                        param.referenceDNSID.length()));

  ASSERT_EQ(param.expectedMatches ? Success : Result::ERROR_BAD_CERT_DOMAIN,
            CheckCertHostname(certInput, hostnameInput));
}

TEST_P(pkixnames_CheckCertHostname_PresentedMatchesReference,
       SubjectAltName_CNNotDNSName)
{
  
  

  const PresentedMatchesReference& param(GetParam());

  ByteString cert(CreateCert(RDN(CN("Common Name")),
                             DNSName(param.presentedDNSID)));
  ASSERT_FALSE(ENCODING_FAILED(cert));
  Input certInput;
  ASSERT_EQ(Success, certInput.Init(cert.data(), cert.length()));

  Input hostnameInput;
  ASSERT_EQ(Success, hostnameInput.Init(param.referenceDNSID.data(),
                                        param.referenceDNSID.length()));
  Result expectedResult
    = param.expectedResult != Success ? param.expectedResult
    : param.expectedMatches ? Success
    : Result::ERROR_BAD_CERT_DOMAIN;
  ASSERT_EQ(expectedResult, CheckCertHostname(certInput, hostnameInput));
}

INSTANTIATE_TEST_CASE_P(pkixnames_CheckCertHostname_DNSID_MATCH_PARAMS,
                        pkixnames_CheckCertHostname_PresentedMatchesReference,
                        testing::ValuesIn(DNSID_MATCH_PARAMS));

TEST_P(pkixnames_Turkish_I_Comparison, CheckCertHostname_CN_NoSAN)
{
  
  
  

  const InputValidity& param(GetParam());
  SCOPED_TRACE(param.input.c_str());

  Input input;
  ASSERT_EQ(Success, input.Init(param.input.data(), param.input.length()));

  ByteString cert(CreateCert(RDN(CN(param.input)), NO_SAN));
  ASSERT_FALSE(ENCODING_FAILED(cert));
  Input certInput;
  ASSERT_EQ(Success, certInput.Init(cert.data(), cert.length()));

  Result expectedResult = (InputsAreEqual(LOWERCASE_I, input) ||
                           InputsAreEqual(UPPERCASE_I, input))
                        ? Success
                        : Result::ERROR_BAD_CERT_DOMAIN;

  ASSERT_EQ(expectedResult, CheckCertHostname(certInput, UPPERCASE_I));
  ASSERT_EQ(expectedResult, CheckCertHostname(certInput, LOWERCASE_I));
}

TEST_P(pkixnames_Turkish_I_Comparison, CheckCertHostname_SAN)
{
  
  
  

  const InputValidity& param(GetParam());
  SCOPED_TRACE(param.input.c_str());

  Input input;
  ASSERT_EQ(Success, input.Init(param.input.data(), param.input.length()));

  ByteString cert(CreateCert(RDN(CN("Common Name")), DNSName(param.input)));
  ASSERT_FALSE(ENCODING_FAILED(cert));
  Input certInput;
  ASSERT_EQ(Success, certInput.Init(cert.data(), cert.length()));

  Result expectedResult
    = (!param.isValidPresentedID) ? Result::ERROR_BAD_DER
    : (InputsAreEqual(LOWERCASE_I, input) ||
       InputsAreEqual(UPPERCASE_I, input)) ? Success
    : Result::ERROR_BAD_CERT_DOMAIN;

  ASSERT_EQ(expectedResult, CheckCertHostname(certInput, UPPERCASE_I));
  ASSERT_EQ(expectedResult, CheckCertHostname(certInput, LOWERCASE_I));
}

class pkixnames_CheckCertHostname_IPV4_Addresses
  : public ::testing::Test
  , public ::testing::WithParamInterface<IPAddressParams<4>>
{
};

TEST_P(pkixnames_CheckCertHostname_IPV4_Addresses,
       ValidIPv4AddressInIPAddressSAN)
{
  
  

  const IPAddressParams<4>& param(GetParam());

  ByteString cert(CreateCert(RDN(CN("Common Name")),
                             IPAddress(param.expectedValueIfValid)));
  ASSERT_FALSE(ENCODING_FAILED(cert));
  Input certInput;
  ASSERT_EQ(Success, certInput.Init(cert.data(), cert.length()));

  Input hostnameInput;
  ASSERT_EQ(Success, hostnameInput.Init(param.input.data(),
                                        param.input.length()));

  ASSERT_EQ(param.isValid ? Success : Result::ERROR_BAD_CERT_DOMAIN,
            CheckCertHostname(certInput, hostnameInput));
}

TEST_P(pkixnames_CheckCertHostname_IPV4_Addresses,
       ValidIPv4AddressInCN_NoSAN)
{
  
  

  const IPAddressParams<4>& param(GetParam());

  SCOPED_TRACE(param.input.c_str());

  ByteString cert(CreateCert(RDN(CN(param.input)), NO_SAN));
  ASSERT_FALSE(ENCODING_FAILED(cert));
  Input certInput;
  ASSERT_EQ(Success, certInput.Init(cert.data(), cert.length()));

  Input hostnameInput;
  ASSERT_EQ(Success, hostnameInput.Init(param.input.data(),
                                        param.input.length()));

  
  Result expectedResult = (param.isValid || IsValidReferenceDNSID(hostnameInput))
                        ? Success
                        : Result::ERROR_BAD_CERT_DOMAIN;

  ASSERT_EQ(expectedResult, CheckCertHostname(certInput, hostnameInput));
}

INSTANTIATE_TEST_CASE_P(pkixnames_CheckCertHostname_IPV4_ADDRESSES,
                        pkixnames_CheckCertHostname_IPV4_Addresses,
                        testing::ValuesIn(IPV4_ADDRESSES));

struct NameConstraintParams
{
  ByteString subject;
  ByteString subjectAltName;
  ByteString subtrees;
  Result expectedPermittedSubtreesResult;
  Result expectedExcludedSubtreesResult;
};

static ByteString
PermittedSubtrees(const ByteString& generalSubtrees)
{
  return TLV(der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 0,
             generalSubtrees);
}

static ByteString
ExcludedSubtrees(const ByteString& generalSubtrees)
{
  return TLV(der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 1,
             generalSubtrees);
}


static ByteString
GeneralSubtree(const ByteString& base)
{
  return TLV(der::SEQUENCE, base);
}

static const NameConstraintParams NAME_CONSTRAINT_PARAMS[] =
{
  
  
  
  { ByteString(), NO_SAN,
    GeneralSubtree(DNSName("!")),
    Success, Success
  },
  { 
    
    ByteString(), NO_SAN,
    GeneralSubtree(Name(ByteString(reinterpret_cast<const uint8_t*>("!"), 1))),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER
  },
  { ByteString(), NO_SAN,
    GeneralSubtree(IPAddress(ipv4_constraint_truncated_bytes)),
    Success, Success
  },
  { ByteString(), NO_SAN,
    GeneralSubtree(IPAddress(ipv4_constraint_overlong_bytes)),
    Success, Success
  },
  { ByteString(), NO_SAN,
  GeneralSubtree(IPAddress(ipv6_constraint_truncated_bytes)),
  Success, Success
  },
  { ByteString(), NO_SAN,
  GeneralSubtree(IPAddress(ipv6_constraint_overlong_bytes)),
  Success, Success
  },
  { ByteString(), NO_SAN,
    GeneralSubtree(RFC822Name("!")),
    Success, Success
  },

  
  
  
  

  
  
  
  { ByteString(), DNSName("host.example.com"),
    GeneralSubtree(DNSName("host.example.com")),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    ByteString(), DNSName("host1.example.com"),
    GeneralSubtree(DNSName("host.example.com")),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Success
  },
  { ByteString(), RFC822Name("a@host.example.com"),
    GeneralSubtree(RFC822Name("host.example.com")),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    ByteString(), RFC822Name("a@host1.example.com"),
    GeneralSubtree(RFC822Name("host.example.com")),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Success
  },

  
  
  
  { 
    ByteString(),  DNSName("www.host.example.com"),
    GeneralSubtree(DNSName(    "host.example.com")),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    
    ByteString(),  RFC822Name("a@www.host.example.com"),
    GeneralSubtree(RFC822Name(      "host.example.com")),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE,
    Success
  },

  
  
  
  { ByteString(), DNSName("bigfoo.bar.com"),
    GeneralSubtree(DNSName(  "foo.bar.com")),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Success
  },
  { ByteString(), RFC822Name("a@bigfoo.bar.com"),
    GeneralSubtree(RFC822Name(    "foo.bar.com")),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Success
  },

  
  
  
  
  { ByteString(), DNSName("www.example.com"),
    GeneralSubtree(DNSName(  ".example.com")),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    
    ByteString(), RFC822Name("a@www.example.com"),
    GeneralSubtree(RFC822Name(    ".example.com")),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    
    ByteString(), RFC822Name("a@www.example.com"),
    GeneralSubtree(RFC822Name(  "a@.example.com")),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER
  },
  { 
    ByteString(), DNSName(  "example.com"),
    GeneralSubtree(DNSName(".example.com")),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Success
  },
  { 
    ByteString(), RFC822Name("a@example.com"),
    GeneralSubtree(RFC822Name(".example.com")),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Success
  },
  { 
    ByteString(), DNSName("bexample.com"),
    GeneralSubtree(DNSName(".example.com")),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Success
  },
  { 
    ByteString(), RFC822Name("a@bexample.com"),
    GeneralSubtree(RFC822Name( ".example.com")),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Success
  },

  
  
  

  
  
  
  
  { 
    
    ByteString(), DNSName("example.com"),
    GeneralSubtree(DNSName("example.com.")),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER,
  },
  { ByteString(), RFC822Name("a@example.com"),
    GeneralSubtree(RFC822Name( "example.com.")),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER,
  },
  { 
    
    ByteString(), DNSName("example.com."),
    GeneralSubtree(DNSName("example.com")),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER,
  },
  { ByteString(), RFC822Name("a@example.com."),
    GeneralSubtree(RFC822Name( "example.com")),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER,
  },
  { 
    
    
    
    ByteString(), DNSName("p.example.com"),
    GeneralSubtree(DNSName(".example.com.")),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER,
  },
  { 
    
    
    ByteString(), RFC822Name("a@p.example.com"),
    GeneralSubtree(RFC822Name(  ".example.com.")),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER,
  },
  { 
    ByteString(), DNSName("*.example.com"),
    GeneralSubtree(DNSName(".example.com.")),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER
  },
  { 
    
    ByteString(), RFC822Name("a@*.example.com"),
    GeneralSubtree(RFC822Name(  ".example.com.")),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER
  },

  
  { ByteString(), DNSName("example.com"),
    GeneralSubtree(DNSName("")),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { ByteString(), RFC822Name("a@example.com"),
    GeneralSubtree(RFC822Name("")),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    ByteString(), DNSName("example.com."),
    GeneralSubtree(DNSName("")),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER
  },
  { ByteString(), RFC822Name("a@example.com."),
    GeneralSubtree(RFC822Name("")),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER
  },
  { 
    ByteString(), DNSName("example.com"),
    GeneralSubtree(DNSName(".")),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER,
  },
  { 
    ByteString(), RFC822Name("a@example.com"),
    GeneralSubtree(RFC822Name(".")),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER,
  },
  { ByteString(), DNSName("example.com."),
    GeneralSubtree(DNSName(".")),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER
  },
  { ByteString(), RFC822Name("a@example.com."),
    GeneralSubtree(RFC822Name(".")),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER
  },

  
  

  
  { ByteString(), IPAddress(ipv4_addr_bytes),
    GeneralSubtree(IPAddress(ipv4_constraint_all_zeros_bytes)),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { ByteString(), IPAddress(ipv4_addr_00000000_bytes),
    GeneralSubtree(IPAddress(ipv4_constraint_all_zeros_bytes)),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { ByteString(), IPAddress(ipv4_addr_FFFFFFFF_bytes),
    GeneralSubtree(IPAddress(ipv4_constraint_all_zeros_bytes)),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },

  
  { ByteString(), IPAddress(ipv6_addr_bytes),
    GeneralSubtree(IPAddress(ipv6_constraint_all_zeros_bytes)),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { ByteString(), IPAddress(ipv6_addr_all_zeros_bytes),
    GeneralSubtree(IPAddress(ipv6_constraint_all_zeros_bytes)),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },

  
  
  
  { ByteString(), IPAddress(ipv4_addr_bytes),
    GeneralSubtree(IPAddress(ipv6_constraint_all_zeros_bytes)),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Success
  },
  { ByteString(), IPAddress(ipv6_addr_bytes),
    GeneralSubtree(IPAddress(ipv4_constraint_all_zeros_bytes)),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Success
  },

  
  { ByteString(), IPAddress(ipv4_addr_bytes),
    GeneralSubtree(IPAddress(ipv4_constraint_CIDR_16_bytes)),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { ByteString(), IPAddress(ipv4_addr_bytes),
    GeneralSubtree(IPAddress(ipv4_constraint_CIDR_17_bytes)),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { ByteString(), IPAddress(ipv4_other_addr_bytes),
    GeneralSubtree(IPAddress(ipv4_constraint_CIDR_16_bytes)),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Success
  },
  { 
    ByteString(), IPAddress(ipv4_addr_bytes),
    GeneralSubtree(IPAddress(ipv4_constraint_CIDR_16_bad_addr_bytes)),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    ByteString(), IPAddress(ipv4_other_addr_bytes),
    GeneralSubtree(IPAddress(ipv4_constraint_bad_mask_bytes)),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Success
  },

  
  { ByteString(), IPAddress(ipv6_addr_bytes),
    GeneralSubtree(IPAddress(ipv6_constraint_CIDR_16_bytes)),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { ByteString(), IPAddress(ipv6_other_addr_bytes),
    GeneralSubtree(IPAddress(ipv6_constraint_CIDR_16_bytes)),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Success
  },
  { 
    ByteString(), IPAddress(ipv6_addr_bytes),
    GeneralSubtree(IPAddress(ipv6_constraint_CIDR_16_bad_addr_bytes)),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    ByteString(), IPAddress(ipv6_other_addr_bytes),
    GeneralSubtree(IPAddress(ipv6_constraint_bad_mask_bytes)),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Success
  },

  

  { 
    ByteString(), IPAddress(),
    GeneralSubtree(IPAddress(ipv4_constraint_all_zeros_bytes)),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER
  },
  { 
    ByteString(), IPAddress(ipv4_addr_truncated_bytes),
    GeneralSubtree(IPAddress(ipv4_constraint_all_zeros_bytes)),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER
  },
  { 
    ByteString(), IPAddress(ipv4_addr_overlong_bytes),
    GeneralSubtree(IPAddress(ipv4_constraint_all_zeros_bytes)),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER
  },
  { 
    ByteString(), IPAddress(ipv4_addr_bytes),
    GeneralSubtree(IPAddress()),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER
  },
  { 
    ByteString(), IPAddress(ipv4_addr_bytes),
    GeneralSubtree(IPAddress(ipv4_constraint_truncated_bytes)),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER
  },
  { 
    ByteString(), IPAddress(ipv4_addr_bytes),
    GeneralSubtree(IPAddress(ipv4_constraint_overlong_bytes)),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER
  },
  { 
    ByteString(), IPAddress(),
    GeneralSubtree(IPAddress(ipv6_constraint_all_zeros_bytes)),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER
  },
  { 
    ByteString(), IPAddress(ipv6_addr_truncated_bytes),
    GeneralSubtree(IPAddress(ipv6_constraint_all_zeros_bytes)),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER
  },
  { 
    ByteString(), IPAddress(ipv6_addr_overlong_bytes),
    GeneralSubtree(IPAddress(ipv6_constraint_all_zeros_bytes)),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER
  },
  { 
    ByteString(), IPAddress(ipv6_addr_bytes),
    GeneralSubtree(IPAddress()),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER
  },
  { 
    ByteString(), IPAddress(ipv6_addr_bytes),
    GeneralSubtree(IPAddress(ipv6_constraint_truncated_bytes)),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER
  },
  { 
    ByteString(), IPAddress(ipv6_addr_bytes),
    GeneralSubtree(IPAddress(ipv6_constraint_overlong_bytes)),
    Result::ERROR_BAD_DER, Result::ERROR_BAD_DER
  },

  
  
  
  { ByteString(), NO_SAN, GeneralSubtree(DNSName("!")),
    Success, Success
  },
  { ByteString(), NO_SAN, GeneralSubtree(IPAddress(ipv4_addr_overlong_bytes)),
    Success, Success
  },
  { ByteString(), NO_SAN, GeneralSubtree(IPAddress(ipv6_addr_overlong_bytes)),
    Success, Success
  },
  { ByteString(), NO_SAN, GeneralSubtree(RFC822Name("\0")),
    Success, Success
  },

  
  

  { 
    ByteString(), NO_SAN, GeneralSubtree(DNSName("a.example.com")),
    Success, Success
  },
  { 
    
    
    
    RDN(CN("")), NO_SAN, GeneralSubtree(DNSName("a.example.com")),
    Success, Success
  },
  { 
    
    
    RDN(CN("1.2.3.4")), NO_SAN, GeneralSubtree(DNSName("a.example.com")),
    Success, Success
  },
  { 
    RDN(OU("a.example.com")), NO_SAN, GeneralSubtree(DNSName("a.example.com")),
    Success, Success
  },
  { 
    RDN(OU("b.example.com")), NO_SAN, GeneralSubtree(DNSName("a.example.com")),
    Success, Success
  },
  { 
    RDN(CN("Not a DNSName")), NO_SAN, GeneralSubtree(DNSName("a.example.com")),
    Success, Success
  },
  { RDN(CN("a.example.com")), NO_SAN, GeneralSubtree(DNSName("a.example.com")),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { RDN(CN("b.example.com")), NO_SAN, GeneralSubtree(DNSName("a.example.com")),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Success
  },
  { 
    
    RDN(CN("a.example.com")), RFC822Name("foo@example.com"),
    GeneralSubtree(DNSName("a.example.com")),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    
    RDN(CN("a.example.com")), RFC822Name("foo@example.com"),
    GeneralSubtree(DNSName("b.example.com")),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Success
  },
  { 
    RDN(CN("a.example.com")), DNSName("b.example.com"),
    GeneralSubtree(DNSName("a.example.com")),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Success
  },
  { 
    RDN(CN("a.example.com")), DNSName("b.example.com"),
    GeneralSubtree(DNSName("b.example.com")),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE,
  },
  { 
    RDN(CN("a.example.com")), IPAddress(ipv4_addr_bytes),
    GeneralSubtree(DNSName("a.example.com")),
    Success, Success
  },
  { 
    RDN(CN("a.example.com")), IPAddress(ipv4_addr_bytes),
    GeneralSubtree(DNSName("b.example.com")),
    Success, Success
  },

  { 
    
    RDN(CN(ipv4_addr_str)), RFC822Name("foo@example.com"),
    GeneralSubtree(IPAddress(ipv4_addr_bytes_FFFFFFFF)),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    
    RDN(CN(ipv4_addr_str)), RFC822Name("foo@example.com"),
    GeneralSubtree(IPAddress(ipv4_other_addr_bytes_FFFFFFFF)),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Success
  },
  { 
    RDN(CN(ipv4_addr_str)), DNSName("b.example.com"),
    GeneralSubtree(IPAddress(ipv4_addr_bytes_FFFFFFFF)),
    Success, Success
  },
  { 
    RDN(CN(ipv4_addr_str)), DNSName("b.example.com"),
    GeneralSubtree(IPAddress(ipv4_addr_bytes_FFFFFFFF)),
    Success, Success
  },
  { 
    RDN(CN(ipv4_addr_str)), IPAddress(ipv4_other_addr_bytes),
    GeneralSubtree(IPAddress(ipv4_addr_bytes_FFFFFFFF)),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Success
  },
  { 
    RDN(CN(ipv4_addr_str)), IPAddress(ipv4_other_addr_bytes),
    GeneralSubtree(IPAddress(ipv4_other_addr_bytes_FFFFFFFF)),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },

  
  
  

  { 
    
    RDN(CN("a.example.com") + CN("b.example.com")), NO_SAN,
    GeneralSubtree(DNSName("a.example.com")),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Success
  },
  { 
    
    RDN(CN("a.example.com")) + RDN(CN("b.example.com")), NO_SAN,
    GeneralSubtree(DNSName("a.example.com")),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Success
  },
  { 
    
    RDN(CN("a.example.com") + CN("b.example.com")), NO_SAN,
    GeneralSubtree(DNSName("b.example.com")),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    
    RDN(CN("a.example.com")) + RDN(CN("b.example.com")), NO_SAN,
    GeneralSubtree(DNSName("b.example.com")),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },

  
  
  
  

  { ByteString(), RFC822Name("a@example.com"),
    GeneralSubtree(RFC822Name("a@example.com")),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },

  
  
  { ByteString(), RFC822Name("a@example.com"),
    GeneralSubtree(RFC822Name("@example.com")),
    Result::ERROR_BAD_DER,
    Result::ERROR_BAD_DER
  },
  { ByteString(), RFC822Name("@example.com"),
    GeneralSubtree(RFC822Name("@example.com")),
    Result::ERROR_BAD_DER,
    Result::ERROR_BAD_DER
  },
  { ByteString(), RFC822Name("example.com"),
    GeneralSubtree(RFC822Name("@example.com")),
    Result::ERROR_BAD_DER,
    Result::ERROR_BAD_DER
  },
  { ByteString(), RFC822Name("a@mail.example.com"),
    GeneralSubtree(RFC822Name("a@*.example.com")),
    Result::ERROR_BAD_DER,
    Result::ERROR_BAD_DER
  },
  { ByteString(), RFC822Name("a@*.example.com"),
    GeneralSubtree(RFC822Name(".example.com")),
    Result::ERROR_BAD_DER,
    Result::ERROR_BAD_DER
  },
  { ByteString(), RFC822Name("@example.com"),
    GeneralSubtree(RFC822Name(".example.com")),
    Result::ERROR_BAD_DER,
    Result::ERROR_BAD_DER
  },
  { ByteString(), RFC822Name("@a.example.com"),
    GeneralSubtree(RFC822Name(".example.com")),
    Result::ERROR_BAD_DER,
    Result::ERROR_BAD_DER
  },

  
  
  
  { ByteString(), DNSName("uses_underscore.example.com"),
    GeneralSubtree(DNSName("uses_underscore.example.com")),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { ByteString(), DNSName("uses_underscore.example.com"),
    GeneralSubtree(DNSName("example.com")),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { ByteString(), DNSName("a.uses_underscore.example.com"),
    GeneralSubtree(DNSName("uses_underscore.example.com")),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { ByteString(), RFC822Name("a@uses_underscore.example.com"),
    GeneralSubtree(RFC822Name("uses_underscore.example.com")),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { ByteString(), RFC822Name("uses_underscore@example.com"),
    GeneralSubtree(RFC822Name("example.com")),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { ByteString(), RFC822Name("a@a.uses_underscore.example.com"),
    GeneralSubtree(RFC822Name(".uses_underscore.example.com")),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },

  
  
  
  
  { 
    RDN(CN("a.example.com")), ByteString(),
    GeneralSubtree(DNSName("a.example.com")),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    
    
    
    
    
    
    
    RDN(emailAddress("a@example.com")), ByteString(),
    GeneralSubtree(RFC822Name("a@example.com")),
    Success, Success
  },
  { 
    
    RDN(emailAddress("a@example.com")), NO_SAN,
    GeneralSubtree(RFC822Name("a@example.com")),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },

  
  

  { 
    RDN(OU("Example Organization")) + RDN(CN("example.com")), NO_SAN,
    GeneralSubtree(DirectoryName(Name(RDN(OU("Example Organization")) +
                                      RDN(CN("example.com"))))),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    RDN(OU("Example Organization") + CN("example.com")), NO_SAN,
    GeneralSubtree(DirectoryName(Name(RDN(OU("Example Organization") +
                                          CN("example.com"))))),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    RDN(OU("Example Organization")) + RDN(CN("example.com")), NO_SAN,
    GeneralSubtree(DirectoryName(Name(RDN(OU("Example Organization"))))),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    
    
    RDN(OU("Other Example Organization")) + RDN(CN("example.com")), NO_SAN,
    GeneralSubtree(DirectoryName(Name(RDN(OU("Example Organization")) +
                                      RDN(CN("example.com"))))),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    RDN(OU("Other Example Organization") + CN("example.com")), NO_SAN,
    GeneralSubtree(DirectoryName(Name(RDN(OU("Example Organization") +
                                          CN("example.com"))))),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    
    RDN(OU("Example Organization") + CN("example.com")), NO_SAN,
    GeneralSubtree(DirectoryName(Name(RDN(OU("Example Organization"))))),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    
    RDN(OU("Example Organization") + CN("example.com")), NO_SAN,
    GeneralSubtree(DirectoryName(Name(RDN(OU("Example Organization")) +
                                      RDN(CN("example.com"))))),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    
    RDN(OU("Example Organization")) + RDN(CN("example.com")), NO_SAN,
    GeneralSubtree(DirectoryName(Name(RDN(OU("Example Organization") +
                                          CN("example.com"))))),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    
    
    RDN(OU("Example Organization", der::UTF8String)) + RDN(CN("example.com")),
    NO_SAN, GeneralSubtree(DirectoryName(Name(RDN(OU("Example Organization",
                                                     der::PrintableString)) +
                                              RDN(CN("example.com"))))),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    RDN(OU("Example Organization", der::PrintableString)) + RDN(CN("example.com")),
    NO_SAN, GeneralSubtree(DirectoryName(Name(RDN(OU("Example Organization",
                                                     der::UTF8String)) +
                                              RDN(CN("example.com"))))),
    Success, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    RDN(OU("Other Example Organization", der::UTF8String)) + RDN(CN("example.com")),
    NO_SAN, GeneralSubtree(DirectoryName(Name(RDN(OU("Example Organization",
                                                     der::PrintableString)) +
                                              RDN(CN("example.com"))))),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
  { 
    RDN(OU("Example Organization", der::PrintableString)) + RDN(CN("example.com")),
    NO_SAN, GeneralSubtree(DirectoryName(Name(RDN(OU("Example Organization",
                                                     der::TeletexString)) +
                                              RDN(CN("example.com"))))),
    Result::ERROR_CERT_NOT_IN_NAME_SPACE, Result::ERROR_CERT_NOT_IN_NAME_SPACE
  },
};

class pkixnames_CheckNameConstraints
  : public ::testing::Test
  , public ::testing::WithParamInterface<NameConstraintParams>
{
};

TEST_P(pkixnames_CheckNameConstraints,
       NameConstraintsEnforcedForDirectlyIssuedEndEntity)
{
  
  

  const NameConstraintParams& param(GetParam());

  ByteString certDER(CreateCert(param.subject, param.subjectAltName));
  ASSERT_FALSE(ENCODING_FAILED(certDER));
  Input certInput;
  ASSERT_EQ(Success, certInput.Init(certDER.data(), certDER.length()));
  BackCert cert(certInput, EndEntityOrCA::MustBeEndEntity, nullptr);
  ASSERT_EQ(Success, cert.Init());

  {
    ByteString nameConstraintsDER(TLV(der::SEQUENCE,
                                      PermittedSubtrees(param.subtrees)));
    Input nameConstraints;
    ASSERT_EQ(Success,
              nameConstraints.Init(nameConstraintsDER.data(),
                                   nameConstraintsDER.length()));
    ASSERT_EQ(param.expectedPermittedSubtreesResult,
              CheckNameConstraints(nameConstraints, cert,
                                   KeyPurposeId::id_kp_serverAuth));
  }
  {
    ByteString nameConstraintsDER(TLV(der::SEQUENCE,
                                      ExcludedSubtrees(param.subtrees)));
    Input nameConstraints;
    ASSERT_EQ(Success,
              nameConstraints.Init(nameConstraintsDER.data(),
                                   nameConstraintsDER.length()));
    ASSERT_EQ(param.expectedExcludedSubtreesResult,
              CheckNameConstraints(nameConstraints, cert,
                                   KeyPurposeId::id_kp_serverAuth));
  }
  {
    ByteString nameConstraintsDER(TLV(der::SEQUENCE,
                                      PermittedSubtrees(param.subtrees) +
                                      ExcludedSubtrees(param.subtrees)));
    Input nameConstraints;
    ASSERT_EQ(Success,
              nameConstraints.Init(nameConstraintsDER.data(),
                                   nameConstraintsDER.length()));
    ASSERT_EQ((param.expectedPermittedSubtreesResult ==
               param.expectedExcludedSubtreesResult)
                ? param.expectedExcludedSubtreesResult
                : Result::ERROR_CERT_NOT_IN_NAME_SPACE,
              CheckNameConstraints(nameConstraints, cert,
                                   KeyPurposeId::id_kp_serverAuth));
  }
}

INSTANTIATE_TEST_CASE_P(pkixnames_CheckNameConstraints,
                        pkixnames_CheckNameConstraints,
                        testing::ValuesIn(NAME_CONSTRAINT_PARAMS));
