





































gTestfile = '15.1.1.1.js';











var SECTION = "15.1.1.1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "NaN";

writeHeaderToLog( SECTION + " "+ TITLE);


new TestCase( SECTION, "NaN",               Number.NaN,     NaN );
new TestCase( SECTION, "this.NaN",          Number.NaN,     this.NaN );
new TestCase( SECTION, "typeof NaN",        "number",       typeof NaN );

test();
