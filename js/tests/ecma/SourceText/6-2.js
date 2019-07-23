





































gTestfile = '6-2.js';










































var SECTION = "6-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Source Text";

writeHeaderToLog( SECTION + " "+ TITLE);



new TestCase(  SECTION,
	       "var s = 'PAS\\u0022SED'; s",
	       "PAS\"SED",
	       eval("var s = 'PAS\\u0022SED'; s") );

new TestCase(  SECTION,
	       'var s = "PAS\\u0022SED"; s',
	       "PAS\"SED",
	       eval('var s = "PAS\\u0022SED"; s') );


new TestCase(  SECTION,
	       "var s = 'PAS\\u0027SED'; s",
	       "PAS\'SED",
	       eval("var s = 'PAS\\u0027SED'; s") );


new TestCase(  SECTION,
	       'var s = "PAS\\u0027SED"; s',
	       "PAS\'SED",
	       eval('var s = "PAS\\u0027SED"; s') );

var testcase =  new TestCase( SECTION,
			      'var s="PAS\\u0027SED"; s',
			      "PAS\'SED",
			      "" );
var s = "PAS\u0027SED";

testcase.actual =  s;

testcase = new TestCase(  SECTION,
			  'var s = "PAS\\u0022SED"; s',
			  "PAS\"SED",
			  "" );
var s = "PAS\u0022SED";

testcase.actual = s;


test();

