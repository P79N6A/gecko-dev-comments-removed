





































gTestfile = '15.1.1.2.js';











var SECTION = "15.1.1.2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Infinity";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION, "Infinity",               Number.POSITIVE_INFINITY,      Infinity );
new TestCase( SECTION, "this.Infinity",          Number.POSITIVE_INFINITY,      this.Infinity );
new TestCase( SECTION, "typeof Infinity",        "number",                      typeof Infinity );

test();
