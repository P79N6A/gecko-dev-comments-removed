





































gTestfile = '11.6.1-2.js';










































var SECTION = "11.6.1-2";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " The Addition operator ( + )");




new TestCase(   SECTION,
                "var EXP_1 = new MyValuelessObject('string'); var EXP_2 = new MyValuelessObject(false); EXP_1 + EXP_2",
                "stringfalse",
                eval("var EXP_1 = new MyValuelessObject('string'); var EXP_2 = new MyValuelessObject(false); EXP_1 + EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new MyValuelessObject(new String('string')); var EXP_2 = new MyValuelessObject(new Boolean(false)); EXP_1 + EXP_2",
                "stringfalse",
                eval("var EXP_1 = new MyValuelessObject(new String('string')); var EXP_2 = new MyValuelessObject(new Boolean(false)); EXP_1 + EXP_2") );




new TestCase(   SECTION,
                "var EXP_1 = new MyValuelessObject(100); var EXP_2 = new MyValuelessObject('string'); EXP_1 + EXP_2",
                "100string",
                eval("var EXP_1 = new MyValuelessObject(100); var EXP_2 = new MyValuelessObject('string'); EXP_1 + EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new MyValuelessObject(new String('string')); var EXP_2 = new MyValuelessObject(new Number(-1)); EXP_1 + EXP_2",
                "string-1",
                eval("var EXP_1 = new MyValuelessObject(new String('string')); var EXP_2 = new MyValuelessObject(new Number(-1)); EXP_1 + EXP_2") );

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
