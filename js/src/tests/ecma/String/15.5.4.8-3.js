





































gTestfile = '15.5.4.8-3.js';


























































var SECTION = "15.5.4.8-3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.prototype.split";

writeHeaderToLog( SECTION + " "+ TITLE);

var TEST_STRING = "";
var EXPECT = new Array();



new TestCase(   SECTION,
		"var s = new String(); s.split().length",
		1,
		eval("var s = new String(); s.split().length") );

new TestCase(   SECTION,
		"var s = new String(); s.split()[0]",
		"",
		eval("var s = new String(); s.split()[0]") );



new TestCase(   SECTION,
		"var s = new String(); s.split('').length",
		0,
		eval("var s = new String(); s.split('').length") );

new TestCase(   SECTION,
		"var s = new String(); s.split(' ').length",
		1,
		eval("var s = new String(); s.split(' ').length") );


new TestCase(   SECTION,
		"var s = new String(' '); s.split().length",
		1,
		eval("var s = new String(' '); s.split().length") );

new TestCase(   SECTION,
		"var s = new String(' '); s.split()[0]",
		" ",
		eval("var s = new String(' '); s.split()[0]") );

new TestCase(   SECTION,
		"var s = new String(' '); s.split('').length",
		1,
		eval("var s = new String(' '); s.split('').length") );

new TestCase(   SECTION,
		"var s = new String(' '); s.split('')[0]",
		" ",
		eval("var s = new String(' '); s.split('')[0]") );

new TestCase(   SECTION,
		"var s = new String(' '); s.split(' ').length",
		2,
		eval("var s = new String(' '); s.split(' ').length") );

new TestCase(   SECTION,
		"var s = new String(' '); s.split(' ')[0]",
		"",
		eval("var s = new String(' '); s.split(' ')[0]") );

new TestCase(   SECTION,
		"\"\".split(\"\").length",
		0,
		("".split("")).length );

new TestCase(   SECTION,
		"\"\".split(\"x\").length",
		1,
		("".split("x")).length );

new TestCase(   SECTION,
		"\"\".split(\"x\")[0]",
		"",
		("".split("x"))[0] );

test();

function Split( string, separator ) {
  string = String( string );

  var A = new Array();

  if ( arguments.length < 2 ) {
    A[0] = string;
    return A;
  }

  separator = String( separator );

  var str_len = String( string ).length;
  var sep_len = String( separator ).length;

  var p = 0;
  var k = 0;

  if ( sep_len == 0 ) {
    for ( ; p < str_len; p++ ) {
      A[A.length] = String( string.charAt(p) );
    }
  }
  return A;
}
