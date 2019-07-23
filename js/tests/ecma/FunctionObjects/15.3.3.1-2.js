





































gTestfile = '15.3.3.1-2.js';


















var SECTION = "15.3.3.1-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Function.prototype";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase(   SECTION,
		"var str='';for (prop in Function ) str += prop; str;",
		"",
		eval("var str='';for (prop in Function) str += prop; str;")
  );
test();
