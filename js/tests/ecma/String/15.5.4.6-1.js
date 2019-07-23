





































gTestfile = '15.5.4.6-1.js';









































var SECTION = "15.5.4.6-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.protoype.indexOf";

var TEST_STRING = new String( " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" );

writeHeaderToLog( SECTION + " "+ TITLE);

var j = 0;

for ( k = 0, i = 0x0020; i < 0x007e; i++, j++, k++ ) {
  new TestCase( SECTION,
		"String.indexOf(" +String.fromCharCode(i)+ ", 0)",
		k,
		TEST_STRING.indexOf( String.fromCharCode(i), 0 ) );
}

for ( k = 0, i = 0x0020; i < 0x007e; i++, j++, k++ ) {
  new TestCase( SECTION,
		"String.indexOf("+String.fromCharCode(i)+ ", "+ k +")",
		k,
		TEST_STRING.indexOf( String.fromCharCode(i), k ) );
}

for ( k = 0, i = 0x0020; i < 0x007e; i++, j++, k++ ) {
  new TestCase( SECTION,
		"String.indexOf("+String.fromCharCode(i)+ ", "+k+1+")",
		-1,
		TEST_STRING.indexOf( String.fromCharCode(i), k+1 ) );
}

for ( k = 0, i = 0x0020; i < 0x007d; i++, j++, k++ ) {
  new TestCase( SECTION,
		"String.indexOf("+(String.fromCharCode(i) +
				   String.fromCharCode(i+1)+
				   String.fromCharCode(i+2)) +", "+0+")",
		k,
		TEST_STRING.indexOf( (String.fromCharCode(i)+
				      String.fromCharCode(i+1)+
				      String.fromCharCode(i+2)),
				     0 ) );
}

for ( k = 0, i = 0x0020; i < 0x007d; i++, j++, k++ ) {
  new TestCase( SECTION,
		"String.indexOf("+(String.fromCharCode(i) +
				   String.fromCharCode(i+1)+
				   String.fromCharCode(i+2)) +", "+ k +")",
		k,
		TEST_STRING.indexOf( (String.fromCharCode(i)+
				      String.fromCharCode(i+1)+
				      String.fromCharCode(i+2)),
				     k ) );
}
for ( k = 0, i = 0x0020; i < 0x007d; i++, j++, k++ ) {
  new TestCase( SECTION,
		"String.indexOf("+(String.fromCharCode(i) +
				   String.fromCharCode(i+1)+
				   String.fromCharCode(i+2)) +", "+ k+1 +")",
		-1,
		TEST_STRING.indexOf( (String.fromCharCode(i)+
				      String.fromCharCode(i+1)+
				      String.fromCharCode(i+2)),
				     k+1 ) );
}

new TestCase( SECTION,  "String.indexOf(" +TEST_STRING + ", 0 )", 0, TEST_STRING.indexOf( TEST_STRING, 0 ) );

new TestCase( SECTION,  "String.indexOf(" +TEST_STRING + ", 1 )", -1, TEST_STRING.indexOf( TEST_STRING, 1 ));

print( "TEST_STRING = new String(\" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\")" );


test();
