





































gTestfile = '11.6.1-2.js';










































var SECTION = "11.6.1-2";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " The Addition operator ( + )");




new TestCase(   SECTION,
                "var EXP_1 = 'string'; var EXP_2 = false; EXP_1 + EXP_2",
                "stringfalse",
                eval("var EXP_1 = 'string'; var EXP_2 = false; EXP_1 + EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = true; var EXP_2 = 'string'; EXP_1 + EXP_2",
                "truestring",
                eval("var EXP_1 = true; var EXP_2 = 'string'; EXP_1 + EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new Boolean(true); var EXP_2 = new String('string'); EXP_1 + EXP_2",
                "truestring",
                eval("var EXP_1 = new Boolean(true); var EXP_2 = new String('string'); EXP_1 + EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new Object(true); var EXP_2 = new Object('string'); EXP_1 + EXP_2",
                "truestring",
                eval("var EXP_1 = new Object(true); var EXP_2 = new Object('string'); EXP_1 + EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new Object(new String('string')); var EXP_2 = new Object(new Boolean(false)); EXP_1 + EXP_2",
                "stringfalse",
                eval("var EXP_1 = new Object(new String('string')); var EXP_2 = new Object(new Boolean(false)); EXP_1 + EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new MyObject(true); var EXP_2 = new MyObject('string'); EXP_1 + EXP_2",
                "truestring",
                eval("var EXP_1 = new MyObject(true); var EXP_2 = new MyObject('string'); EXP_1 + EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new MyObject(new String('string')); var EXP_2 = new MyObject(new Boolean(false)); EXP_1 + EXP_2",
                "[object Object][object Object]",
                eval("var EXP_1 = new MyObject(new String('string')); var EXP_2 = new MyObject(new Boolean(false)); EXP_1 + EXP_2") );




new TestCase(   SECTION,
                "var EXP_1 = 100; var EXP_2 = 'string'; EXP_1 + EXP_2",
                "100string",
                eval("var EXP_1 = 100; var EXP_2 = 'string'; EXP_1 + EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new String('string'); var EXP_2 = new Number(-1); EXP_1 + EXP_2",
                "string-1",
                eval("var EXP_1 = new String('string'); var EXP_2 = new Number(-1); EXP_1 + EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new Object(100); var EXP_2 = new Object('string'); EXP_1 + EXP_2",
                "100string",
                eval("var EXP_1 = new Object(100); var EXP_2 = new Object('string'); EXP_1 + EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new Object(new String('string')); var EXP_2 = new Object(new Number(-1)); EXP_1 + EXP_2",
                "string-1",
                eval("var EXP_1 = new Object(new String('string')); var EXP_2 = new Object(new Number(-1)); EXP_1 + EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new MyObject(100); var EXP_2 = new MyObject('string'); EXP_1 + EXP_2",
                "100string",
                eval("var EXP_1 = new MyObject(100); var EXP_2 = new MyObject('string'); EXP_1 + EXP_2") );

new TestCase(   SECTION,
                "var EXP_1 = new MyObject(new String('string')); var EXP_2 = new MyObject(new Number(-1)); EXP_1 + EXP_2",
                "[object Object][object Object]",
                eval("var EXP_1 = new MyObject(new String('string')); var EXP_2 = new MyObject(new Number(-1)); EXP_1 + EXP_2") );

test();

function MyObject( value ) {
  this.valueOf = new Function( "return this.value" );
  this.value = value;
}
