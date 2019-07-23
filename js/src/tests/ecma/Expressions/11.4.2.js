





































gTestfile = '11.4.2.js';









var SECTION = "11.4.2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The void operator";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,   "void(new String('string object'))",      void 0,  void(new String( 'string object' )) );
new TestCase( SECTION,   "void('string primitive')",               void 0,  void("string primitive") );
new TestCase( SECTION,   "void(Number.NaN)",                       void 0,  void(Number.NaN) );
new TestCase( SECTION,   "void(Number.POSITIVE_INFINITY)",         void 0,  void(Number.POSITIVE_INFINITY) );
new TestCase( SECTION,   "void(1)",                                void 0,  void(1) );
new TestCase( SECTION,   "void(0)",                                void 0,  void(0) );
new TestCase( SECTION,   "void(-1)",                               void 0,  void(-1) );
new TestCase( SECTION,   "void(Number.NEGATIVE_INFINITY)",         void 0,  void(Number.NEGATIVE_INFINITY) );
new TestCase( SECTION,   "void(Math.PI)",                          void 0,  void(Math.PI) );
new TestCase( SECTION,   "void(true)",                             void 0,  void(true) );
new TestCase( SECTION,   "void(false)",                            void 0,  void(false) );
new TestCase( SECTION,   "void(null)",                             void 0,  void(null) );
new TestCase( SECTION,   "void new String('string object')",      void 0,  void new String( 'string object' ) );
new TestCase( SECTION,   "void 'string primitive'",               void 0,  void "string primitive" );
new TestCase( SECTION,   "void Number.NaN",                       void 0,  void Number.NaN );
new TestCase( SECTION,   "void Number.POSITIVE_INFINITY",         void 0,  void Number.POSITIVE_INFINITY );
new TestCase( SECTION,   "void 1",                                void 0,  void 1 );
new TestCase( SECTION,   "void 0",                                void 0,  void 0 );
new TestCase( SECTION,   "void -1",                               void 0,  void -1 );
new TestCase( SECTION,   "void Number.NEGATIVE_INFINITY",         void 0,  void Number.NEGATIVE_INFINITY );
new TestCase( SECTION,   "void Math.PI",                          void 0,  void Math.PI );
new TestCase( SECTION,   "void true",                             void 0,  void true );
new TestCase( SECTION,   "void false",                            void 0,  void false );
new TestCase( SECTION,   "void null",                             void 0,  void null );



test();
