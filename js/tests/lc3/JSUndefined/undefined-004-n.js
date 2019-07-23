

























































var SECTION = "undefined conversion";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
SECTION;
startTest();

var dt = new DT();

var a = new Array();
var i = 0;




DESCRIPTION = "dt.setShort( undefined )";
EXPECTED = "error";

new TestCase(
    "dt.setShort( undefined )",
    "error",
    dt.setShort(undefined) );

test();
