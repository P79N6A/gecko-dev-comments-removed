





































gTestfile = '7.1-1.js';





















var SECTION = "7.1-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "White Space";

writeHeaderToLog( SECTION + " "+ TITLE);



new TestCase( SECTION,  'var'+'\t'+'MYVAR1=10;MYVAR1',   10, eval('var'+'\t'+'MYVAR1=10;MYVAR1') );
new TestCase( SECTION,  'var'+'\f'+'MYVAR2=10;MYVAR2',   10, eval('var'+'\f'+'MYVAR2=10;MYVAR2') );
new TestCase( SECTION,  'var'+'\v'+'MYVAR2=10;MYVAR2',   10, eval('var'+'\v'+'MYVAR2=10;MYVAR2') );
new TestCase( SECTION,  'var'+'\ '+'MYVAR2=10;MYVAR2',   10, eval('var'+'\ '+'MYVAR2=10;MYVAR2') );



new TestCase( SECTION,
	      "var a = new Array(12345); a\t\v\f .\\u0009\\000B\\u000C\\u0020length",
	      12345,
	      eval("var a = new Array(12345); a\t\v\f .\u0009\u0020\u000C\u000Blength") );

test();
