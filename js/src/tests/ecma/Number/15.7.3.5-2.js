





































gTestfile = '15.7.3.5-2.js';













var SECTION = "15.7.3.5-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Number.NEGATIVE_INFINITY";

writeHeaderToLog( SECTION + " "+TITLE);

new TestCase(   SECTION,
		"delete( Number.NEGATIVE_INFINITY )",
		false,
		eval("delete( Number.NEGATIVE_INFINITY )") );

new TestCase(   SECTION,
		"delete( Number.NEGATIVE_INFINITY ); Number.NEGATIVE_INFINITY",
		-Infinity,
		eval("delete( Number.NEGATIVE_INFINITY );Number.NEGATIVE_INFINITY") );

test();
