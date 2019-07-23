





































gTestfile = '8.6.2.1-1.js';





















var SECTION = "8.6.2.1-1";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " [[Get]] (Value)");

new TestCase( SECTION,  "var OBJ = new MyObject(true); OBJ.valueOf()",              true,           eval("var OBJ = new MyObject(true); OBJ.valueOf()") );

new TestCase( SECTION,  "var OBJ = new MyObject(Number.POSITIVE_INFINITY); OBJ.valueOf()",              Number.POSITIVE_INFINITY,           eval("var OBJ = new MyObject(Number.POSITIVE_INFINITY); OBJ.valueOf()") );

new TestCase( SECTION,  "var OBJ = new MyObject('string'); OBJ.valueOf()",              'string',           eval("var OBJ = new MyObject('string'); OBJ.valueOf()") );

test();

function MyObject( value ) {
  this.valueOf = new Function( "return this.value" );
  this.value = value;
}
