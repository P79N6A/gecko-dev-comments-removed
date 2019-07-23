





































gTestfile = '15.3.3.1-4.js';


















var SECTION = "15.3.3.1-4";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Function.prototype";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase(   SECTION,
		"Function.prototype = null; Function.prototype",
		Function.prototype,
		eval("Function.prototype = null; Function.prototype")
  );
test();
