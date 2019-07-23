





































gTestfile = '7.1-3.js';





















var SECTION = "7.1-3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "White Space";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,    "'var'+'\u000B'+'MYVAR1=10;MYVAR1'",   10, eval('var'+'\u000B'+'MYVAR1=10;MYVAR1') );
new TestCase( SECTION,    "'var'+'\u0009'+'MYVAR2=10;MYVAR2'",   10, eval('var'+'\u0009'+'MYVAR2=10;MYVAR2') );
new TestCase( SECTION,    "'var'+'\u000C'+'MYVAR3=10;MYVAR3'",   10, eval('var'+'\u000C'+'MYVAR3=10;MYVAR3') );
new TestCase( SECTION,    "'var'+'\u0020'+'MYVAR4=10;MYVAR4'",   10, eval('var'+'\u0020'+'MYVAR4=10;MYVAR4') );



new TestCase(   SECTION,
		"var VAR = 12345; + + VAR",
		12345,
		eval("var VAR = 12345; + + VAR") );

new TestCase(   SECTION,
		"var VAR = 12345;VAR+ + VAR",
		24690,
		eval("var VAR = 12345;VAR+ +VAR") );
new TestCase(   SECTION,
		"var VAR = 12345;VAR - - VAR",
		24690,
		eval("var VAR = 12345;VAR- -VAR") );

test();
