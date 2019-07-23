





































gTestfile = '9.4-2.js';

































var SECTION = "9.4-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "ToInteger";

writeHeaderToLog( SECTION + " "+ TITLE);



new TestCase( SECTION,  "td = new Date(Number.NaN); td.valueOf()",  Number.NaN, eval("td = new Date(Number.NaN); td.valueOf()") );
new TestCase( SECTION,  "td = new Date(Infinity); td.valueOf()",    Number.NaN, eval("td = new Date(Number.POSITIVE_INFINITY); td.valueOf()") );
new TestCase( SECTION,  "td = new Date(-Infinity); td.valueOf()",   Number.NaN, eval("td = new Date(Number.NEGATIVE_INFINITY); td.valueOf()") );
new TestCase( SECTION,  "td = new Date(-0); td.valueOf()",          -0,         eval("td = new Date(-0); td.valueOf()" ) );
new TestCase( SECTION,  "td = new Date(0); td.valueOf()",           0,          eval("td = new Date(0); td.valueOf()") );



new TestCase( SECTION,  "td = new Date(3.14159); td.valueOf()",     3,          eval("td = new Date(3.14159); td.valueOf()") );
new TestCase( SECTION,  "td = new Date(Math.PI); td.valueOf()",     3,          eval("td = new Date(Math.PI); td.valueOf()") );
new TestCase( SECTION,  "td = new Date(-Math.PI);td.valueOf()",     -3,         eval("td = new Date(-Math.PI);td.valueOf()") );
new TestCase( SECTION,  "td = new Date(3.14159e2); td.valueOf()",   314,        eval("td = new Date(3.14159e2); td.valueOf()") );

new TestCase( SECTION,  "td = new Date(.692147e1); td.valueOf()",   6,          eval("td = new Date(.692147e1);td.valueOf()") );
new TestCase( SECTION,  "td = new Date(-.692147e1);td.valueOf()",   -6,         eval("td = new Date(-.692147e1);td.valueOf()") );



new TestCase( SECTION,  "td = new Date(true); td.valueOf()",        1,          eval("td = new Date(true); td.valueOf()" ) );
new TestCase( SECTION,  "td = new Date(false); td.valueOf()",       0,          eval("td = new Date(false); td.valueOf()") );

new TestCase( SECTION,  "td = new Date(new Number(Math.PI)); td.valueOf()",  3, eval("td = new Date(new Number(Math.PI)); td.valueOf()") );
new TestCase( SECTION,  "td = new Date(new Number(Math.PI)); td.valueOf()",  3, eval("td = new Date(new Number(Math.PI)); td.valueOf()") );


new TestCase( SECTION,  "td = new Date(8.64e15); td.valueOf()",     8.64e15,    eval("td = new Date(8.64e15); td.valueOf()") );
new TestCase( SECTION,  "td = new Date(-8.64e15); td.valueOf()",    -8.64e15,   eval("td = new Date(-8.64e15); td.valueOf()") );
new TestCase( SECTION,  "td = new Date(8.64e-15); td.valueOf()",    0,          eval("td = new Date(8.64e-15); td.valueOf()") );
new TestCase( SECTION,  "td = new Date(-8.64e-15); td.valueOf()",   0,          eval("td = new Date(-8.64e-15); td.valueOf()") );

test();
