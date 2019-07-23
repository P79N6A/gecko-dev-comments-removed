





































gTestfile = '11.6.2-1.js';






















var SECTION = "11.6.2-1";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " The subtraction operator ( - )");




new TestCase(   SECTION,
                "var EXP_1 = true; var EXP_2 = false; EXP_1 - EXP_2",
                1,
                eval("var EXP_1 = true; var EXP_2 = false; EXP_1 - EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new Boolean(true); var EXP_2 = new Boolean(false); EXP_1 - EXP_2",
                1,
                eval("var EXP_1 = new Boolean(true); var EXP_2 = new Boolean(false); EXP_1 - EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new Object(true); var EXP_2 = new Object(false); EXP_1 - EXP_2",
                1,
                eval("var EXP_1 = new Object(true); var EXP_2 = new Object(false); EXP_1 - EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new Object(new Boolean(true)); var EXP_2 = new Object(new Boolean(false)); EXP_1 - EXP_2",
                1,
                eval("var EXP_1 = new Object(new Boolean(true)); var EXP_2 = new Object(new Boolean(false)); EXP_1 - EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new MyObject(true); var EXP_2 = new MyObject(false); EXP_1 - EXP_2",
                1,
                eval("var EXP_1 = new MyObject(true); var EXP_2 = new MyObject(false); EXP_1 - EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new MyObject(new Boolean(true)); var EXP_2 = new MyObject(new Boolean(false)); EXP_1 - EXP_2",
                Number.NaN,
                eval("var EXP_1 = new MyObject(new Boolean(true)); var EXP_2 = new MyObject(new Boolean(false)); EXP_1 - EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new MyOtherObject(new Boolean(true)); var EXP_2 = new MyOtherObject(new Boolean(false)); EXP_1 - EXP_2",
                Number.NaN,
                eval("var EXP_1 = new MyOtherObject(new Boolean(true)); var EXP_2 = new MyOtherObject(new Boolean(false)); EXP_1 - EXP_2") );




new TestCase(   SECTION,
                "var EXP_1 = 100; var EXP_2 = 1; EXP_1 - EXP_2",
                99,
                eval("var EXP_1 = 100; var EXP_2 = 1; EXP_1 - EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new Number(100); var EXP_2 = new Number(1); EXP_1 - EXP_2",
                99,
                eval("var EXP_1 = new Number(100); var EXP_2 = new Number(1); EXP_1 - EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new Object(100); var EXP_2 = new Object(1); EXP_1 - EXP_2",
                99,
                eval("var EXP_1 = new Object(100); var EXP_2 = new Object(1); EXP_1 - EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new Object(new Number(100)); var EXP_2 = new Object(new Number(1)); EXP_1 - EXP_2",
                99,
                eval("var EXP_1 = new Object(new Number(100)); var EXP_2 = new Object(new Number(1)); EXP_1 - EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new MyObject(100); var EXP_2 = new MyObject(1); EXP_1 - EXP_2",
                99,
                eval("var EXP_1 = new MyObject(100); var EXP_2 = new MyObject(1); EXP_1 - EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new MyObject(new Number(100)); var EXP_2 = new MyObject(new Number(1)); EXP_1 - EXP_2",
                Number.NaN,
                eval("var EXP_1 = new MyObject(new Number(100)); var EXP_2 = new MyObject(new Number(1)); EXP_1 - EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new MyOtherObject(new Number(100)); var EXP_2 = new MyOtherObject(new Number(1)); EXP_1 - EXP_2",
                99,
                eval("var EXP_1 = new MyOtherObject(new Number(100)); var EXP_2 = new MyOtherObject(new Number(1)); EXP_1 - EXP_2") );


new TestCase(   SECTION,
                "var EXP_1 = new MyOtherObject(new String('0xff')); var EXP_2 = new MyOtherObject(new String('1'); EXP_1 - EXP_2",
                254,
                eval("var EXP_1 = new MyOtherObject(new String('0xff')); var EXP_2 = new MyOtherObject(new String('1')); EXP_1 - EXP_2") );

test();

function MyPrototypeObject(value) {
  this.valueOf = new Function( "return this.value;" );
  this.toString = new Function( "return (this.value + '');" );
  this.value = value;
}
function MyObject( value ) {
  this.valueOf = new Function( "return this.value" );
  this.value = value;
}
function MyOtherObject( value ) {
  this.valueOf = new Function( "return this.value" );
  this.toString = new Function ( "return this.value + ''" );
  this.value = value;
}
