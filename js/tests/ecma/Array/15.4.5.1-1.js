





































gTestfile = '15.4.5.1-1.js';





































var SECTION = "15.4.5.1-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Array [[Put]] (P, V)";

writeHeaderToLog( SECTION + " "+ TITLE);




new TestCase(   SECTION,
		"var A = new Array(); A.length = 1000; A.length",
		1000,
		eval("var A = new Array(); A.length = 1000; A.length") );


new TestCase(   SECTION,
		"var A = new Array(1000); A.name = 'name of this array'; A.name",
		'name of this array',
		eval("var A = new Array(1000); A.name = 'name of this array'; A.name") );

new TestCase(   SECTION,
		"var A = new Array(1000); A.name = 'name of this array'; A.length",
		1000,
		eval("var A = new Array(1000); A.name = 'name of this array'; A.length") );





new TestCase(   SECTION,
		"var A = new Array(1000); A[123] = 'hola'; A[123]",
		'hola',
		eval("var A = new Array(1000); A[123] = 'hola'; A[123]") );

new TestCase(   SECTION,
		"var A = new Array(1000); A[123] = 'hola'; A.length",
		1000,
		eval("var A = new Array(1000); A[123] = 'hola'; A.length") );


for ( var i = 0X0020, TEST_STRING = "var A = new Array( " ; i < 0x00ff; i++ ) {
  TEST_STRING += "\'\\"+ String.fromCharCode( i ) +"\'";
  if ( i < 0x00FF - 1   ) {
    TEST_STRING += ",";
  } else {
    TEST_STRING += ");"
      }
}

var LENGTH = 0x00ff - 0x0020;

new TestCase(   SECTION,
		TEST_STRING +" A[150] = 'hello'; A[150]",
		'hello',
		eval( TEST_STRING + " A[150] = 'hello'; A[150]" ) );

new TestCase(   SECTION,
		TEST_STRING +" A[150] = 'hello'; A[150]",
		LENGTH,
		eval( TEST_STRING + " A[150] = 'hello'; A.length" ) );




new TestCase(   SECTION,
		"var A = new Array(); A[123] = true; A.length",
		124,
		eval("var A = new Array(); A[123] = true; A.length") );

new TestCase(   SECTION,
		"var A = new Array(0,1,2,3,4,5,6,7,8,9,10); A[15] ='15'; A.length",
		16,
		eval("var A = new Array(0,1,2,3,4,5,6,7,8,9,10); A[15] ='15'; A.length") );

for ( var i = 0; i < A.length; i++ ) {
  new TestCase( SECTION,
		"var A = new Array(0,1,2,3,4,5,6,7,8,9,10); A[15] ='15'; A[" +i +"]",
		(i <= 10) ? i : ( i == 15 ? '15' : void 0 ),
		A[i] );
}


new TestCase(   SECTION,
		"var A = new Array(); A.join.length = 4; A.join.length",
		1,
		eval("var A = new Array(); A.join.length = 4; A.join.length") );

new TestCase(   SECTION,
		"var A = new Array(); A.join.length = 4; A.length",
		0,
		eval("var A = new Array(); A.join.length = 4; A.length") );

test();
