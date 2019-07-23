









































gTestfile = '12.1.js';

START("12.1 - Default XML Namespace");



var soap = new Namespace("http://schemas.xmlsoap.org/soap/envelope/");
var stock = new Namespace("http://mycompany.com/stocks");
default xml namespace = soap;


message =
<Envelope
    xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/"
    soap:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
    <Body>
        <stock:GetLastTradePrice xmlns:stock="http://mycompany.com/stocks">
            <stock:symbol>DIS</stock:symbol>
        </stock:GetLastTradePrice>
    </Body>
</Envelope>;


encodingStyle = message.@soap::encodingStyle;
TEST_XML(1, "http://schemas.xmlsoap.org/soap/encoding/", encodingStyle);


correct =
<Body
    xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
    <stock:GetLastTradePrice xmlns:stock="http://mycompany.com/stocks">
        <stock:symbol>DIS</stock:symbol>
    </stock:GetLastTradePrice>
</Body>;

body = message.Body;
TEST_XML(2, correct.toXMLString(), body);


correct =
<Envelope
    xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/"
    soap:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
    <Body>
        <stock:GetLastTradePrice xmlns:stock="http://mycompany.com/stocks">
            <stock:symbol>MYCO</stock:symbol>
        </stock:GetLastTradePrice>
    </Body>
</Envelope>;

message.Body.stock::GetLastTradePrice.stock::symbol = "MYCO";

TEST(3, correct, message);

function scopeTest()
{
    var x = <a/>;
    TEST(4, soap.uri, x.namespace().uri);

    default xml namespace = "http://" + "someuri.org";
    x = <a/>;
    TEST(5, "http://someuri.org", x.namespace().uri);
}

scopeTest();

x = <a><b><c xmlns="">foo</c></b></a>;
TEST(6, soap.uri, x.namespace().uri);
TEST(7, soap.uri, x.b.namespace().uri);

ns = new Namespace("");
TEST(8, "foo", x.b.ns::c.toString());

x = <a foo="bar"/>;
TEST(9, soap.uri, x.namespace().uri);
TEST(10, "", x.@foo.namespace().uri);
TEST_XML(11, "bar", x.@foo);

default xml namespace = "";
x = <x/>;
ns = new Namespace("http://someuri");
default xml namespace = ns;
x.a = "foo";
TEST(12, "foo", x["a"].toString());
q = new QName("a");
TEST(13, "foo", x[q].toString());

default xml namespace = "";
x[q] = "bar";
TEST(14, "bar", x.ns::a.toString());

END();
