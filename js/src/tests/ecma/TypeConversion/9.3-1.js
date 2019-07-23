





































gTestfile = '9.3-1.js';























var SECTION = "9.3-1";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " ToNumber");


new TestCase( SECTION, "Number(new Number())", 0, Number(new Number()) );
new TestCase( SECTION, "typeof Number(new Number())", "number", typeof Number(new Number()) );

new TestCase( SECTION, "Number(new Number(Number.NaN))", Number.NaN, Number(new Number(Number.NaN)) );
new TestCase( SECTION, "typeof Number(new Number(Number.NaN))","number", typeof Number(new Number(Number.NaN)) );

new TestCase( SECTION, "Number(new Number(0))", 0, Number(new Number(0)) );
new TestCase( SECTION, "typeof Number(new Number(0))", "number", typeof Number(new Number(0)) );

new TestCase( SECTION, "Number(new Number(null))", 0, Number(new Number(null)) );
new TestCase( SECTION, "typeof Number(new Number(null))", "number", typeof Number(new Number(null)) );



new TestCase( SECTION, "Number(new Number(true))", 1, Number(new Number(true)) );
new TestCase( SECTION, "typeof Number(new Number(true))", "number", typeof Number(new Number(true)) );

new TestCase( SECTION, "Number(new Number(false))", 0, Number(new Number(false)) );
new TestCase( SECTION, "typeof Number(new Number(false))", "number", typeof Number(new Number(false)) );


new TestCase( SECTION, "Number(new Boolean(true))", 1, Number(new Boolean(true)) );
new TestCase( SECTION, "typeof Number(new Boolean(true))", "number", typeof Number(new Boolean(true)) );

new TestCase( SECTION, "Number(new Boolean(false))", 0, Number(new Boolean(false)) );
new TestCase( SECTION, "typeof Number(new Boolean(false))", "number", typeof Number(new Boolean(false)) );


new TestCase( SECTION, "Number(new Array(2,4,8,16,32))", Number.NaN, Number(new Array(2,4,8,16,32)) );
new TestCase( SECTION, "typeof Number(new Array(2,4,8,16,32))", "number", typeof Number(new Array(2,4,8,16,32)) );

test();
