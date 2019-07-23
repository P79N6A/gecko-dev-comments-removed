





































gTestfile = 'regexparg-2-n.js';













var SECTION = "JS_1.2";
var VERSION = "JS_1.2";
startTest();
var TITLE   = "The variable statement";

writeHeaderToLog( SECTION + " "+ TITLE);

function f(x) {return x;}

x = f(/abc/);

DESCRIPTION = "function f(x) {return x;}; x = f(/abc/); x()";
EXPECTED = "error";

new TestCase( SECTION,
	      "function f(x) {return x;}; x = f(/abc/); x()",
	      "error",
	      x() );

test();

