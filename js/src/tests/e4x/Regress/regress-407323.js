





































gTestfile = 'regress-407323.js';

var summary = 'XML, XMLList, QName are mutable, Namespace is not.';
var BUGNUMBER = 407323;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var obj           = {};
var saveQName     = QName;
var saveXML       = XML;
var saveXMLList   = XMLList;
var saveNamespace = Namespace;

QName = obj;
TEST(1, obj, QName);

XML = obj;
TEST(2, obj, XML);

XMLList = obj;
TEST(3, obj, XMLList);

Namespace = obj;
TEST(4, obj, Namespace);

END();
