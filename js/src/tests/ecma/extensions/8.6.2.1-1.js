





































gTestfile = '8.6.2.1-1.js';





















var SECTION = "8.6.2.1-1";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " [[Get]] (Value)");

new TestCase( SECTION,  "var OBJ = new MyValuelessObject(true); OBJ.valueOf()",     true,           eval("var OBJ = new MyValuelessObject(true); OBJ.valueOf()") );

new TestCase( SECTION,  "var OBJ = new MyProtolessObject(true); OBJ.valueOf()",     true,           eval("var OBJ = new MyProtolessObject(true); OBJ.valueOf()") );

new TestCase( SECTION,  "var OBJ = new MyValuelessObject(Number.POSITIVE_INFINITY); OBJ.valueOf()",     Number.POSITIVE_INFINITY,           eval("var OBJ = new MyValuelessObject(Number.POSITIVE_INFINITY); OBJ.valueOf()") );

new TestCase( SECTION,  "var OBJ = new MyProtolessObject(Number.POSITIVE_INFINITY); OBJ.valueOf()",     Number.POSITIVE_INFINITY,           eval("var OBJ = new MyProtolessObject(Number.POSITIVE_INFINITY); OBJ.valueOf()") );

new TestCase( SECTION,  "var OBJ = new MyValuelessObject('string'); OBJ.valueOf()",     'string',           eval("var OBJ = new MyValuelessObject('string'); OBJ.valueOf()") );

new TestCase( SECTION,  "var OBJ = new MyProtolessObject('string'); OBJ.valueOf()",     'string',           eval("var OBJ = new MyProtolessObject('string'); OBJ.valueOf()") );

test();

function MyProtoValuelessObject(value) {
  this.valueOf = new Function ( "" );
  this.__proto__ = null;
}

function MyProtolessObject( value ) {
  this.valueOf = new Function( "return this.value" );
  this.__proto__ = null;
  this.value = value;
}
function MyValuelessObject(value) {
  this.__proto__ = new MyPrototypeObject(value);
}
function MyPrototypeObject(value) {
  this.valueOf = new Function( "return this.value;" );
  this.toString = new Function( "return (this.value + '');" );
  this.value = value;
}
