





































gTestfile = '11.6.1-3.js';










































var SECTION = "11.6.1-3";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " The Addition operator ( + )");




var DATE1 = new Date();

new TestCase(   SECTION,
                "var DATE1 = new Date(); DATE1 + DATE1",
                DATE1.toString() + DATE1.toString(),
                DATE1 + DATE1 );

new TestCase(   SECTION,
                "var DATE1 = new Date(); DATE1 + 0",
                DATE1.toString() + 0,
                DATE1 + 0 );

new TestCase(   SECTION,
                "var DATE1 = new Date(); DATE1 + new Number(0)",
                DATE1.toString() + 0,
                DATE1 + new Number(0) );

new TestCase(   SECTION,
                "var DATE1 = new Date(); DATE1 + true",
                DATE1.toString() + "true",
                DATE1 + true );

new TestCase(   SECTION,
                "var DATE1 = new Date(); DATE1 + new Boolean(true)",
                DATE1.toString() + "true",
                DATE1 + new Boolean(true) );

new TestCase(   SECTION,
                "var DATE1 = new Date(); DATE1 + new Boolean(true)",
                DATE1.toString() + "true",
                DATE1 + new Boolean(true) );

var MYOB1 = new MyObject( DATE1 );

new TestCase(   SECTION,
                "MYOB1 = new MyObject(DATE1); MYOB1 + new Number(1)",
                "[object Object]1",
                MYOB1 + new Number(1) );

new TestCase(   SECTION,
                "MYOB1 = new MyObject(DATE1); MYOB1 + 1",
                "[object Object]1",
                MYOB1 + 1 );

new TestCase(   SECTION,
                "MYOB1 = new MyObject(DATE1); MYOB1 + true",
                "[object Object]true",
                MYOB1 + true );

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
