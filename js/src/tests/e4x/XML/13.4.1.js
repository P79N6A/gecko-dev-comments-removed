









































gTestfile = '13.4.1.js';

START("13.4.1 - XML Constructor as Function");

x = XML();
TEST(1, "xml", typeof(x));
TEST(2, true, x instanceof XML);

correct =
<Envelope
    xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/"
    xmlns:stock="http://mycompany.com/stocks"
    soap:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
    <Body>
        <stock:GetLastTradePrice>
            <stock:symbol>DIS</stock:symbol>
        </stock:GetLastTradePrice>
    </Body>
</Envelope>;

x = XML(correct);
TEST(3, correct, x);

text =
"<Envelope" +
"    xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\"" +
"    xmlns:stock=\"http://mycompany.com/stocks\"" +
"    soap:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">" +
"    <Body>" +
"        <stock:GetLastTradePrice>" +
"            <stock:symbol>DIS</stock:symbol>" +
"        </stock:GetLastTradePrice>" +
"    </Body>" +
"</Envelope>";

x =  XML(text);
TEST(4, correct, x);


x =
<alpha>
    <bravo>two</bravo>
</alpha>;

y = XML(x);

x.bravo = "three";

correct =
<alpha>
    <bravo>three</bravo>
</alpha>;

TEST(5, correct, y);


x = XML("4");
TEST_XML(6, 4, x);

x = XML(4);
TEST_XML(7, 4, x);


x = XML(null);
TEST_XML(8, "", x);

x = XML(undefined);
TEST_XML(9, "", x);
 

END();
