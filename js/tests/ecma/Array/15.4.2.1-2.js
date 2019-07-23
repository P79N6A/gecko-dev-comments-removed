





































gTestfile = '15.4.2.1-2.js';




























var SECTION = "15.4.2.1-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The Array Constructor:  new Array( item0, item1, ...)";

writeHeaderToLog( SECTION + " "+ TITLE);


var TEST_STRING = "new Array(";
var ARGUMENTS = ""
  var TEST_LENGTH = Math.pow(2,10); 

for ( var index = 0; index < TEST_LENGTH; index++ ) {
  ARGUMENTS += index;
  ARGUMENTS += (index == (TEST_LENGTH-1) ) ? "" : ",";
}

TEST_STRING += ARGUMENTS + ")";

TEST_ARRAY = eval( TEST_STRING );

for ( var item = 0; item < TEST_LENGTH; item++ ) {
  new TestCase( SECTION,
		"["+item+"]",    
		item,   
		TEST_ARRAY[item] );
}

new TestCase( SECTION,
	      "new Array( ["+TEST_LENGTH+" arguments] ) +''",   
	      ARGUMENTS,
	      TEST_ARRAY +"" );

test();
