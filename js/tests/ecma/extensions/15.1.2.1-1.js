





































gTestfile = '15.1.2.1-1.js';










var SECTION = "15.1.2.1-1";
var VERSION = "ECMA_1";
var TITLE   = "eval(x)";
var BUGNUMBER = "none";

startTest();

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,      "eval.length",              1,              eval.length );
new TestCase( SECTION,      "delete eval.length",       false,          delete eval.length );
new TestCase( SECTION,      "var PROPS = ''; for ( p in eval ) { PROPS += p }; PROPS",  "", eval("var PROPS = ''; for ( p in eval ) { PROPS += p }; PROPS") );
new TestCase( SECTION,      "eval.length = null; eval.length",       1, eval( "eval.length = null; eval.length") );




new TestCase( SECTION,     "eval()",                                void 0,                     eval() );
new TestCase( SECTION,     "eval(void 0)",                          void 0,                     eval( void 0) );
new TestCase( SECTION,     "eval(null)",                            null,                       eval( null ) );
new TestCase( SECTION,     "eval(true)",                            true,                       eval( true ) );
new TestCase( SECTION,     "eval(false)",                           false,                      eval( false ) );

new TestCase( SECTION,     "typeof eval(new String('Infinity/-0')", "object",                   typeof eval(new String('Infinity/-0')) );

new TestCase( SECTION,     "eval([1,2,3,4,5,6])",                  "1,2,3,4,5,6",                 ""+eval([1,2,3,4,5,6]) );
new TestCase( SECTION,     "eval(new Array(0,1,2,3)",              "1,2,3",                       ""+  eval(new Array(1,2,3)) );
new TestCase( SECTION,     "eval(1)",                              1,                             eval(1) );
new TestCase( SECTION,     "eval(0)",                              0,                             eval(0) );
new TestCase( SECTION,     "eval(-1)",                             -1,                            eval(-1) );
new TestCase( SECTION,     "eval(Number.NaN)",                     Number.NaN,                    eval(Number.NaN) );
new TestCase( SECTION,     "eval(Number.MIN_VALUE)",               5e-308,                        eval(Number.MIN_VALUE) );
new TestCase( SECTION,     "eval(-Number.MIN_VALUE)",              -5e-308,                       eval(-Number.MIN_VALUE) );
new TestCase( SECTION,     "eval(Number.POSITIVE_INFINITY)",       Number.POSITIVE_INFINITY,      eval(Number.POSITIVE_INFINITY) );
new TestCase( SECTION,     "eval(Number.NEGATIVE_INFINITY)",       Number.NEGATIVE_INFINITY,      eval(Number.NEGATIVE_INFINITY) );
new TestCase( SECTION,     "eval( 4294967296 )",                   4294967296,                    eval(4294967296) );
new TestCase( SECTION,     "eval( 2147483648 )",                   2147483648,                    eval(2147483648) );

test();
