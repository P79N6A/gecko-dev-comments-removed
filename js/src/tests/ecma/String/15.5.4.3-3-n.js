





































gTestfile = '15.5.4.3-3-n.js';

















var SECTION = "15.5.4.3-3-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.prototype.valueOf";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var valof=String.prototype.valueOf; astring=new Number(); astring.valueOf = valof; astring.valof()";
EXPECTED = "error";

new TestCase( SECTION,
	      "var valof=String.prototype.valueOf; astring=new Number(); astring.valueOf = valof; astring.valof()",
	      "error",
	      eval("var valof=String.prototype.valueOf; astring=new Number(); astring.valueOf = valof; astring.valueOf()") );

test();
