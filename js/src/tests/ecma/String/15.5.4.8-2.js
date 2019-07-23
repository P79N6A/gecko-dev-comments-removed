





































gTestfile = '15.5.4.8-2.js';


























































var SECTION = "15.5.4.8-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.prototype.split";

writeHeaderToLog( SECTION + " "+ TITLE);



var TEST_STRING = "this is a string object";

new TestCase(   SECTION,
		"var s = new String( "+ TEST_STRING +" ); s.split('').length",
		TEST_STRING.length,
		eval("var s = new String( TEST_STRING ); s.split('').length") );

for ( var i = 0; i < TEST_STRING.length; i++ ) {

  new TestCase(   SECTION,
		  "var s = new String( "+TEST_STRING+" ); s.split('')["+i+"]",
		  TEST_STRING.charAt(i),
		  eval("var s = new String( TEST_STRING ); s.split('')["+i+"]") );
}




var TEST_STRING = "thisundefinedisundefinedaundefinedstringundefinedobject";
var EXPECT_STRING = new Array( "this", "is", "a", "string", "object" );

new TestCase(   SECTION,
		"var s = new String( "+ TEST_STRING +" ); s.split(void 0).length",
		EXPECT_STRING.length,
		eval("var s = new String( TEST_STRING ); s.split(void 0).length") );

for ( var i = 0; i < EXPECT_STRING.length; i++ ) {
  new TestCase(   SECTION,
		  "var s = new String( "+TEST_STRING+" ); s.split(void 0)["+i+"]",
		  EXPECT_STRING[i],
		  eval("var s = new String( TEST_STRING ); s.split(void 0)["+i+"]") );
}


TEST_STRING = "thisnullisnullanullstringnullobject";
var EXPECT_STRING = new Array( "this", "is", "a", "string", "object" );

new TestCase(   SECTION,
		"var s = new String( "+ TEST_STRING +" ); s.split(null).length",
		EXPECT_STRING.length,
		eval("var s = new String( TEST_STRING ); s.split(null).length") );

for ( var i = 0; i < EXPECT_STRING.length; i++ ) {
  new TestCase(   SECTION,
		  "var s = new String( "+TEST_STRING+" ); s.split(null)["+i+"]",
		  EXPECT_STRING[i],
		  eval("var s = new String( TEST_STRING ); s.split(null)["+i+"]") );
}


TEST_STRING = "thistrueistrueatruestringtrueobject";
var EXPECT_STRING = new Array( "this", "is", "a", "string", "object" );

new TestCase(   SECTION,
		"var s = new String( "+ TEST_STRING +" ); s.split(true).length",
		EXPECT_STRING.length,
		eval("var s = new String( TEST_STRING ); s.split(true).length") );

for ( var i = 0; i < EXPECT_STRING.length; i++ ) {
  new TestCase(   SECTION,
		  "var s = new String( "+TEST_STRING+" ); s.split(true)["+i+"]",
		  EXPECT_STRING[i],
		  eval("var s = new String( TEST_STRING ); s.split(true)["+i+"]") );
}


TEST_STRING = "this123is123a123string123object";
var EXPECT_STRING = new Array( "this", "is", "a", "string", "object" );

new TestCase(   SECTION,
		"var s = new String( "+ TEST_STRING +" ); s.split(123).length",
		EXPECT_STRING.length,
		eval("var s = new String( TEST_STRING ); s.split(123).length") );

for ( var i = 0; i < EXPECT_STRING.length; i++ ) {
  new TestCase(   SECTION,
		  "var s = new String( "+TEST_STRING+" ); s.split(123)["+i+"]",
		  EXPECT_STRING[i],
		  eval("var s = new String( TEST_STRING ); s.split(123)["+i+"]") );
}



TEST_STRING = "this123is123a123string123object";
var EXPECT_STRING = new Array( "this", "is", "a", "string", "object" );

new TestCase(   SECTION,
		"var s = new String( "+ TEST_STRING +" ); s.split(123).length",
		EXPECT_STRING.length,
		eval("var s = new String( TEST_STRING ); s.split(123).length") );

for ( var i = 0; i < EXPECT_STRING.length; i++ ) {
  new TestCase(   SECTION,
		  "var s = new String( "+TEST_STRING+" ); s.split(123)["+i+"]",
		  EXPECT_STRING[i],
		  eval("var s = new String( TEST_STRING ); s.split(123)["+i+"]") );
}


TEST_STRING = "this is a string";
EXPECT_STRING = new Array( "this is a string" );

new TestCase(   SECTION,
		"var s = new String( " + TEST_STRING + " ); s.split(':').length",
		1,
		eval("var s = new String( TEST_STRING ); s.split(':').length") );

new TestCase(   SECTION,
		"var s = new String( " + TEST_STRING + " ); s.split(':')[0]",
		TEST_STRING,
		eval("var s = new String( TEST_STRING ); s.split(':')[0]") );


TEST_STRING = "this is a string";
EXPECT_STRING = new Array( "this is a string" );
new TestCase(   SECTION,
		"var s = new String( " + TEST_STRING + " ); s.split('strings').length",
		1,
		eval("var s = new String( TEST_STRING ); s.split('strings').length") );

new TestCase(   SECTION,
		"var s = new String( " + TEST_STRING + " ); s.split('strings')[0]",
		TEST_STRING,
		eval("var s = new String( TEST_STRING ); s.split('strings')[0]") );


TEST_STRING = "this is a string";
EXPECT_STRING = new Array( "this is a " );
new TestCase(   SECTION,
		"var s = new String( " + TEST_STRING + " ); s.split('string').length",
		2,
		eval("var s = new String( TEST_STRING ); s.split('string').length") );

for ( var i = 0; i < EXPECT_STRING.length; i++ ) {
  new TestCase(   SECTION,
		  "var s = new String( "+TEST_STRING+" ); s.split('string')["+i+"]",
		  EXPECT_STRING[i],
		  eval("var s = new String( TEST_STRING ); s.split('string')["+i+"]") );
}

test();
