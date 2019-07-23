





































gTestfile = '11.6.1-3.js';










































var SECTION = "11.6.1-3";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " The Addition operator ( + )");





var DATE1 = new Date();

var MYOB1 = new MyObject( DATE1 );
var MYOB2 = new MyValuelessObject( DATE1 );
var MYOB3 = new MyProtolessObject( DATE1 );
var MYOB4 = new MyProtoValuelessObject( DATE1 );

new TestCase(   SECTION,
                "MYOB2 = new MyValuelessObject(DATE1); MYOB3 + 'string'",
                DATE1.toString() + "string",
                MYOB2 + 'string' );

new TestCase(   SECTION,
                "MYOB2 = new MyValuelessObject(DATE1); MYOB3 + new String('string')",
                DATE1.toString() + "string",
                MYOB2 + new String('string') );







test();

function MyProtoValuelessObject() {
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
function MyObject( value ) {
  this.valueOf = new Function( "return this.value" );
  this.value = value;
}
