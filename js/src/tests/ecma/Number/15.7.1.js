





































gTestfile = '15.7.1.js';




















var SECTION = "15.7.1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The Number Constructor Called as a Function";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase(SECTION, "Number()",                  0,          Number() );
new TestCase(SECTION, "Number(void 0)",            Number.NaN,  Number(void 0) );
new TestCase(SECTION, "Number(null)",              0,          Number(null) );
new TestCase(SECTION, "Number()",                  0,          Number() );
new TestCase(SECTION, "Number(new Number())",      0,          Number( new Number() ) );
new TestCase(SECTION, "Number(0)",                 0,          Number(0) );
new TestCase(SECTION, "Number(1)",                 1,          Number(1) );
new TestCase(SECTION, "Number(-1)",                -1,         Number(-1) );
new TestCase(SECTION, "Number(NaN)",               Number.NaN, Number( Number.NaN ) );
new TestCase(SECTION, "Number('string')",          Number.NaN, Number( "string") );
new TestCase(SECTION, "Number(new String())",      0,          Number( new String() ) );
new TestCase(SECTION, "Number('')",                0,          Number( "" ) );
new TestCase(SECTION, "Number(Infinity)",          Number.POSITIVE_INFINITY,   Number("Infinity") );

new TestCase(SECTION, "Number(new MyObject(100))",  100,        Number(new MyObject(100)) );

test();

function MyObject( value ) {
  this.value = value;
  this.valueOf = new Function( "return this.value" );
}
