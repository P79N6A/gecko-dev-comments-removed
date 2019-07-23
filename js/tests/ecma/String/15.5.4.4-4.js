





































gTestfile = '15.5.4.4-4.js';





























var SECTION = "15.5.4.4-4";
var VERSION = "ECMA_2";
startTest();
var TITLE   = "String.prototype.charAt";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,     "x = new Array(1,2,3); x.charAt = String.prototype.charAt; x.charAt(0)",            "1",     eval("x=new Array(1,2,3); x.charAt = String.prototype.charAt; x.charAt(0)") );
new TestCase( SECTION,     "x = new Array(1,2,3); x.charAt = String.prototype.charAt; x.charAt(1)",            ",",     eval("x=new Array(1,2,3); x.charAt = String.prototype.charAt; x.charAt(1)") );
new TestCase( SECTION,     "x = new Array(1,2,3); x.charAt = String.prototype.charAt; x.charAt(2)",            "2",     eval("x=new Array(1,2,3); x.charAt = String.prototype.charAt; x.charAt(2)") );
new TestCase( SECTION,     "x = new Array(1,2,3); x.charAt = String.prototype.charAt; x.charAt(3)",            ",",     eval("x=new Array(1,2,3); x.charAt = String.prototype.charAt; x.charAt(3)") );
new TestCase( SECTION,     "x = new Array(1,2,3); x.charAt = String.prototype.charAt; x.charAt(4)",            "3",     eval("x=new Array(1,2,3); x.charAt = String.prototype.charAt; x.charAt(4)") );

new TestCase( SECTION,  "x = new Array(); x.charAt = String.prototype.charAt; x.charAt(0)",                    "",      eval("x = new Array(); x.charAt = String.prototype.charAt; x.charAt(0)") );

new TestCase( SECTION,     "x = new Number(123); x.charAt = String.prototype.charAt; x.charAt(0)",            "1",     eval("x=new Number(123); x.charAt = String.prototype.charAt; x.charAt(0)") );
new TestCase( SECTION,     "x = new Number(123); x.charAt = String.prototype.charAt; x.charAt(1)",            "2",     eval("x=new Number(123); x.charAt = String.prototype.charAt; x.charAt(1)") );
new TestCase( SECTION,     "x = new Number(123); x.charAt = String.prototype.charAt; x.charAt(2)",            "3",     eval("x=new Number(123); x.charAt = String.prototype.charAt; x.charAt(2)") );

new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(0)",            "[",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(0)") );
new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(1)",            "o",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(1)") );
new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(2)",            "b",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(2)") );
new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(3)",            "j",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(3)") );
new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(4)",            "e",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(4)") );
new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(5)",            "c",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(5)") );
new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(6)",            "t",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(6)") );
new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(7)",            " ",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(7)") );
new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(8)",            "O",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(8)") );
new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(9)",            "b",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(9)") );
new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(10)",            "j",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(10)") );
new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(11)",            "e",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(11)") );
new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(12)",            "c",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(12)") );
new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(13)",            "t",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(13)") );
new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(14)",            "]",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(14)") );

new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(0)",            "[",    eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(0)") );
new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(1)",            "o",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(1)") );
new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(2)",            "b",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(2)") );
new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(3)",            "j",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(3)") );
new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(4)",            "e",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(4)") );
new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(5)",            "c",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(5)") );
new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(6)",            "t",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(6)") );
new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(7)",            " ",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(7)") );
new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(8)",            "F",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(8)") );
new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(9)",            "u",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(9)") );
new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(10)",            "n",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(10)") );
new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(11)",            "c",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(11)") );
new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(12)",            "t",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(12)") );
new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(13)",            "i",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(13)") );
new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(14)",            "o",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(14)") );
new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(15)",            "n",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(15)") );
new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(16)",            "]",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(16)") );
new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(17)",            "",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(17)") );


test();
