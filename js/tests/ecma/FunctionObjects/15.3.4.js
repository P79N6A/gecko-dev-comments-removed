





































gTestfile = '15.3.4.js';























var SECTION = "15.3.4";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Properties of the Function Prototype Object";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase(   SECTION,
		"var myfunc = Function.prototype; myfunc.toString = Object.prototype.toString; myfunc.toString()",
		"[object Function]",
		eval("var myfunc = Function.prototype; myfunc.toString = Object.prototype.toString; myfunc.toString()"));



new TestCase( SECTION,  "Function.prototype.valueOf",       Object.prototype.valueOf,   Function.prototype.valueOf );
new TestCase( SECTION,  "Function.prototype()",             (void 0),                   Function.prototype() );
new TestCase( SECTION,  "Function.prototype(1,true,false,'string', new Date(),null)",  (void 0), Function.prototype(1,true,false,'string', new Date(),null) );

test();
