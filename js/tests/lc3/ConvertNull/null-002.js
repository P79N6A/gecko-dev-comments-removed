











































var SECTION = "Preferred argument conversion:  null";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
SECTION;
startTest();




new TestCase(
    "java.lang.String[\"valueOf(java.lang.Object)\"](null) +''",
    "null",
    java.lang.String["valueOf(java.lang.Object)"](null) + "" );




new TestCase(
    "java.lang.Boolean.valueOf(null) +''",
    "false",
    java.lang.Boolean.valueOf(null) +"" );




new TestCase(
    "java.lang.Boolean[\"valueOf(java.lang.String)\"](null)",
    "false",
    java.lang.Boolean["valueOf(java.lang.String)"](null) +"" );

test();
