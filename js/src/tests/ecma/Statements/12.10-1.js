





































gTestfile = '12.10-1.js';

































var SECTION = "12.10-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The with statement";

writeHeaderToLog( SECTION + " "+ TITLE);





new TestCase( SECTION,
	      "with( new Number() ) { this +'' }",
	      GLOBAL,
	      eval("with( new Number() ) { this +'' }") );




new TestCase(
  SECTION,
  "var MYOB = new WithObject(true); with (MYOB) { parseInt() }",
  true,
  eval("var MYOB = new WithObject(true); with (MYOB) { parseInt() }") );

new TestCase(
  SECTION,
  "var MYOB = new WithObject(false); with (MYOB) { NaN }",
  false,
  eval("var MYOB = new WithObject(false); with (MYOB) { NaN }") );

new TestCase(
  SECTION,
  "var MYOB = new WithObject(NaN); with (MYOB) { Infinity }",
  Number.NaN,
  eval("var MYOB = new WithObject(NaN); with (MYOB) { Infinity }") );

new TestCase(
  SECTION,
  "var MYOB = new WithObject(false); with (MYOB) { }; Infinity",
  Number.POSITIVE_INFINITY,
  eval("var MYOB = new WithObject(false); with (MYOB) { }; Infinity") );


new TestCase(
  SECTION,
  "var MYOB = new WithObject(0); with (MYOB) { delete Infinity; Infinity }",
  Number.POSITIVE_INFINITY,
  eval("var MYOB = new WithObject(0); with (MYOB) { delete Infinity; Infinity }") );



new TestCase(
  SECTION,
  "var MYOB = new WithObject(0); while (true) { with (MYOB) { Infinity; break; } } Infinity",
  Number.POSITIVE_INFINITY,
  eval("var MYOB = new WithObject(0); while (true) { with (MYOB) { Infinity; break; } } Infinity") );


test();

function WithObject( value ) {
  this.prop1 = 1;
  this.prop2 = new Boolean(true);
  this.prop3 = "a string";
  this.value = value;

  

  this.parseInt = new Function( "return this.value" );
  this.NaN = value;
  this.Infinity = value;
  this.unescape = new Function( "return this.value" );
  this.escape   = new Function( "return this.value" );
  this.eval     = new Function( "return this.value" );
  this.parseFloat = new Function( "return this.value" );
  this.isNaN      = new Function( "return this.value" );
  this.isFinite   = new Function( "return this.value" );
}
