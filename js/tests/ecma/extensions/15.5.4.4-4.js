





































gTestfile = '15.5.4.4-4.js';





























var SECTION = "15.5.4.4-4";
var VERSION = "ECMA_2";
startTest();
var TITLE   = "String.prototype.charAt";

writeHeaderToLog( SECTION + " "+ TITLE);












new TestCase( SECTION,     "x = false; x.__proto.charAt = String.prototype.charAt; x.charAt(0)",            "f",     eval("x=false; x.__proto__.charAt = String.prototype.charAt; x.charAt(0)") );
new TestCase( SECTION,     "x = false; x.__proto.charAt = String.prototype.charAt; x.charAt(1)",            "a",     eval("x=false; x.__proto__.charAt = String.prototype.charAt; x.charAt(1)") );
new TestCase( SECTION,     "x = false; x.__proto.charAt = String.prototype.charAt; x.charAt(2)",            "l",     eval("x=false; x.__proto__.charAt = String.prototype.charAt; x.charAt(2)") );
new TestCase( SECTION,     "x = false; x.__proto.charAt = String.prototype.charAt; x.charAt(3)",            "s",     eval("x=false; x.__proto__.charAt = String.prototype.charAt; x.charAt(3)") );
new TestCase( SECTION,     "x = false; x.__proto.charAt = String.prototype.charAt; x.charAt(4)",            "e",     eval("x=false; x.__proto__.charAt = String.prototype.charAt; x.charAt(4)") );

new TestCase( SECTION,     "x = true; x.__proto.charAt = String.prototype.charAt; x.charAt(0)",            "t",     eval("x=true; x.__proto__.charAt = String.prototype.charAt; x.charAt(0)") );
new TestCase( SECTION,     "x = true; x.__proto.charAt = String.prototype.charAt; x.charAt(1)",            "r",     eval("x=true; x.__proto__.charAt = String.prototype.charAt; x.charAt(1)") );
new TestCase( SECTION,     "x = true; x.__proto.charAt = String.prototype.charAt; x.charAt(2)",            "u",     eval("x=true; x.__proto__.charAt = String.prototype.charAt; x.charAt(2)") );
new TestCase( SECTION,     "x = true; x.__proto.charAt = String.prototype.charAt; x.charAt(3)",            "e",     eval("x=true; x.__proto__.charAt = String.prototype.charAt; x.charAt(3)") );

new TestCase( SECTION,     "x = NaN; x.__proto.charAt = String.prototype.charAt; x.charAt(0)",            "N",     eval("x=NaN; x.__proto__.charAt = String.prototype.charAt; x.charAt(0)") );
new TestCase( SECTION,     "x = NaN; x.__proto.charAt = String.prototype.charAt; x.charAt(1)",            "a",     eval("x=NaN; x.__proto__.charAt = String.prototype.charAt; x.charAt(1)") );
new TestCase( SECTION,     "x = NaN; x.__proto.charAt = String.prototype.charAt; x.charAt(2)",            "N",     eval("x=NaN; x.__proto__.charAt = String.prototype.charAt; x.charAt(2)") );

new TestCase( SECTION,     "x = 123; x.__proto.charAt = String.prototype.charAt; x.charAt(0)",            "1",     eval("x=123; x.__proto__.charAt = String.prototype.charAt; x.charAt(0)") );
new TestCase( SECTION,     "x = 123; x.__proto.charAt = String.prototype.charAt; x.charAt(1)",            "2",     eval("x=123; x.__proto__.charAt = String.prototype.charAt; x.charAt(1)") );
new TestCase( SECTION,     "x = 123; x.__proto.charAt = String.prototype.charAt; x.charAt(2)",            "3",     eval("x=123; x.__proto__.charAt = String.prototype.charAt; x.charAt(2)") );


test();
