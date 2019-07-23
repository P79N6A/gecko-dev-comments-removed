






































gTestfile = '9.2.js';


















var SECTION = "9.2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "ToBoolean";

writeHeaderToLog( SECTION + " "+ TITLE);



new TestCase( SECTION,   "Boolean()",                     false,  Boolean() );
new TestCase( SECTION,   "Boolean(var x)",                false,  Boolean(eval("var x")) );
new TestCase( SECTION,   "Boolean(void 0)",               false,  Boolean(void 0) );
new TestCase( SECTION,   "Boolean(null)",                 false,  Boolean(null) );
new TestCase( SECTION,   "Boolean(false)",                false,  Boolean(false) );
new TestCase( SECTION,   "Boolean(true)",                 true,   Boolean(true) );
new TestCase( SECTION,   "Boolean(0)",                    false,  Boolean(0) );
new TestCase( SECTION,   "Boolean(-0)",                   false,  Boolean(-0) );
new TestCase( SECTION,   "Boolean(NaN)",                  false,  Boolean(Number.NaN) );
new TestCase( SECTION,   "Boolean('')",                   false,  Boolean("") );



new TestCase( SECTION,   "Boolean(Infinity)",             true,   Boolean(Number.POSITIVE_INFINITY) );
new TestCase( SECTION,   "Boolean(-Infinity)",            true,   Boolean(Number.NEGATIVE_INFINITY) );
new TestCase( SECTION,   "Boolean(Math.PI)",              true,   Boolean(Math.PI) );
new TestCase( SECTION,   "Boolean(1)",                    true,   Boolean(1) );
new TestCase( SECTION,   "Boolean(-1)",                   true,   Boolean(-1) );
new TestCase( SECTION,   "Boolean([tab])",                true,   Boolean("\t") );
new TestCase( SECTION,   "Boolean('0')",                  true,   Boolean("0") );
new TestCase( SECTION,   "Boolean('string')",             true,   Boolean("string") );


new TestCase( SECTION,   "Boolean(new String() )",        true,   Boolean(new String()) );
new TestCase( SECTION,   "Boolean(new String('') )",      true,   Boolean(new String("")) );

new TestCase( SECTION,   "Boolean(new Boolean(true))",    true,   Boolean(new Boolean(true)) );
new TestCase( SECTION,   "Boolean(new Boolean(false))",   true,   Boolean(new Boolean(false)) );
new TestCase( SECTION,   "Boolean(new Boolean() )",       true,   Boolean(new Boolean()) );

new TestCase( SECTION,   "Boolean(new Array())",          true,   Boolean(new Array()) );

new TestCase( SECTION,   "Boolean(new Number())",         true,   Boolean(new Number()) );
new TestCase( SECTION,   "Boolean(new Number(-0))",       true,   Boolean(new Number(-0)) );
new TestCase( SECTION,   "Boolean(new Number(0))",        true,   Boolean(new Number(0)) );
new TestCase( SECTION,   "Boolean(new Number(NaN))",      true,   Boolean(new Number(Number.NaN)) );

new TestCase( SECTION,   "Boolean(new Number(-1))",       true,   Boolean(new Number(-1)) );
new TestCase( SECTION,   "Boolean(new Number(Infinity))", true,   Boolean(new Number(Number.POSITIVE_INFINITY)) );
new TestCase( SECTION,   "Boolean(new Number(-Infinity))",true,   Boolean(new Number(Number.NEGATIVE_INFINITY)) );

new TestCase( SECTION,    "Boolean(new Object())",       true,       Boolean(new Object()) );
new TestCase( SECTION,    "Boolean(new Function())",     true,       Boolean(new Function()) );
new TestCase( SECTION,    "Boolean(new Date())",         true,       Boolean(new Date()) );
new TestCase( SECTION,    "Boolean(new Date(0))",         true,       Boolean(new Date(0)) );
new TestCase( SECTION,    "Boolean(Math)",         true,       Boolean(Math) );


new TestCase( SECTION,
              "NaN ? true : false",
              false,
              (NaN ? true : false) );
new TestCase( SECTION,
              "1000 % 0 ? true : false",
              false,
              (1000 % 0 ? true : false) );
new TestCase( SECTION,
              "(function(a,b){ return a % b ? true : false })(1000, 0)",
              false,
              ((function(a,b){ return a % b ? true : false })(1000, 0)) );

new TestCase( SECTION,
              "(function(x) { return !(x) })(0/0)",
              true,
              ((function(x) { return !(x) })(0/0)) );
new TestCase( SECTION,
              "!(0/0)",
              true,
              (!(0/0)) );
test();

