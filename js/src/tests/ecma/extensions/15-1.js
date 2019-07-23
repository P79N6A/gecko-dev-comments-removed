





































gTestfile = '15-1.js';


















var SECTION = "15-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Native ECMAScript Objects";

writeHeaderToLog( SECTION + " "+ TITLE);












new TestCase( SECTION,  "Function.prototype.__proto__ == Object.prototype", true,   Function.prototype.__proto__ == Object.prototype );
new TestCase( SECTION,  "Array.prototype.__proto__ == Object.prototype",    true,   Array.prototype.__proto__ == Object.prototype );
new TestCase( SECTION,  "String.prototype.__proto__ == Object.prototype",   true,   String.prototype.__proto__ == Object.prototype );
new TestCase( SECTION,  "Boolean.prototype.__proto__ == Object.prototype",  true,   Boolean.prototype.__proto__ == Object.prototype );
new TestCase( SECTION,  "Number.prototype.__proto__ == Object.prototype",   true,   Number.prototype.__proto__ == Object.prototype );

new TestCase( SECTION,  "Date.prototype.__proto__ == Object.prototype",     true,   Date.prototype.__proto__ == Object.prototype );
new TestCase( SECTION,  "TestCase.prototype.__proto__ == Object.prototype", true,   TestCase.prototype.__proto__ == Object.prototype );

new TestCase( SECTION,  "MyObject.prototype.__proto__ == Object.prototype", true,   MyObject.prototype.__proto__ == Object.prototype );


test();


function MyObject( value ) {
  this.value = value;
  this.valueOf = new Function( "return this.value" );
}
