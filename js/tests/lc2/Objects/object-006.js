
















































var SECTION = "LiveConnect Objects";
var VERSION = "1_3";
var TITLE   = "Construct a java.lang.Character";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var testcase = new TestCase (
    SECTION,
    "var string = new java.lang.String(\"hi\"); "+
    "var c = new java.lang.Character(string.charAt(0)); String(c.toString())",
    "h",
    "" );

var string = new java.lang.String("hi");
var c = new java.lang.Character( string.charAt(0) );

testcase.actual = String(c.toString());

test();

