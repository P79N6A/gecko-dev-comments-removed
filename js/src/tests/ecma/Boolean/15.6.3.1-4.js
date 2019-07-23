





































gTestfile = '15.6.3.1-4.js';

















var SECTION = "15.6.3.1-4";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Boolean.prototype"
  writeHeaderToLog( SECTION + TITLE );

var BOOL_PROTO = Boolean.prototype;

new TestCase( SECTION,
	      "var BOOL_PROTO = Boolean.prototype; Boolean.prototype=null; Boolean.prototype == BOOL_PROTO",
	      true,
	      eval("var BOOL_PROTO = Boolean.prototype; Boolean.prototype=null; Boolean.prototype == BOOL_PROTO") );

new TestCase( SECTION,
	      "var BOOL_PROTO = Boolean.prototype; Boolean.prototype=null; Boolean.prototype == null",
	      false,
	      eval("var BOOL_PROTO = Boolean.prototype; Boolean.prototype=null; Boolean.prototype == null") );

test();
