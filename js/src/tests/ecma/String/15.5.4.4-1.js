





































gTestfile = '15.5.4.4-1.js';




















var SECTION = "15.5.4.4-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.prototype.charAt";

writeHeaderToLog( SECTION + " "+ TITLE);

var TEST_STRING = new String( " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" );

var item = 0;
var i;

for (  i = 0x0020; i < 0x007e; i++, item++) {
  new TestCase( SECTION,
		"TEST_STRING.charAt("+item+")",
		String.fromCharCode( i ),
		TEST_STRING.charAt( item ) );
}

for ( i = 0x0020; i < 0x007e; i++, item++) {
  new TestCase( SECTION,
		"TEST_STRING.charAt("+item+") == TEST_STRING.substring( "+item +", "+ (item+1) + ")",
		true,
		TEST_STRING.charAt( item )  == TEST_STRING.substring( item, item+1 )
    );
}

new TestCase( SECTION,  "String.prototype.charAt.length",       1,  String.prototype.charAt.length );

print( "TEST_STRING = new String(\" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\")" );

test();

