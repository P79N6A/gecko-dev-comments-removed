





































gTestfile = '11.4.1.js';












var SECTION = "11.4.1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The delete operator";

writeHeaderToLog( SECTION + " "+ TITLE);



new TestCase( SECTION,   "x=new Date();delete x;typeof(x)",        "undefined",    eval("x=new Date();delete x;typeof(x)") );





new TestCase( SECTION,   "delete(Math.PI)",             false,   delete(Math.PI) );





var abc;
new TestCase( SECTION,   "var abc; delete(abc)",        false,   delete abc );

new TestCase(   SECTION,
                "var OB = new MyObject(); for ( p in OB ) { delete p }",
                true,
                eval("var OB = new MyObject(); for ( p in OB ) { delete p }") );

test();

function MyObject() {
  this.prop1 = true;
  this.prop2 = false;
  this.prop3 = null;
  this.prop4 = void 0;
  this.prop5 = "hi";
  this.prop6 = 42;
  this.prop7 = new Date();
  this.prop8 = Math.PI;
}
