





































gTestfile = 'array_split_1.js';












var SECTION = "Free Perl";
var VERSION = "JS1_2";
var TITLE   = "Array.split()";

startTest();

writeHeaderToLog( SECTION + " "+ TITLE);


new TestCase( SECTION,
	      "('a,b,c'.split(',')).length",
	      3,
	      ('a,b,c'.split(',')).length );

new TestCase( SECTION,
	      "('a,b'.split(',')).length",
	      2,
	      ('a,b'.split(',')).length );

new TestCase( SECTION,
	      "('a'.split(',')).length",
	      1,
	      ('a'.split(',')).length );






new TestCase( SECTION,
	      "(''.split(',')).length",
	      0,
	      (''.split(',')).length );

test();
