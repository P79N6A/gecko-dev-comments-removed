





































gTestfile = '15.3.3.1-3.js';


















var SECTION = "15.3.3.1-3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Function.prototype";

writeHeaderToLog( SECTION + " "+ TITLE);


var FUN_PROTO = Function.prototype;

new TestCase(   SECTION,
		"delete Function.prototype",
		false,
		delete Function.prototype
  );

new TestCase(   SECTION,
		"delete Function.prototype; Function.prototype",
		FUN_PROTO,
		eval("delete Function.prototype; Function.prototype")
  );
test();
