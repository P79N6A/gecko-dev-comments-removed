





































gTestfile = '15.5.4.2-2-n.js';


















var SECTION = "15.5.4.2-3-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.prototype.toString";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var tostr=String.prototype.toString; astring=new Number(); astring.toString = tostr; astring.toString()";
EXPECTED = "error";

new TestCase( SECTION,
	      "var tostr=String.prototype.toString; astring=new Number(); astring.toString = tostr; astring.toString()",
	      "error",
	      eval("var tostr=String.prototype.toString; astring=new Number(); astring.toString = tostr; astring.toString()") );

test();
