





































gTestfile = '15.4.2.1-3.js';



































var SECTION = "15.4.2.1-3";
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
		"TEST_ARRAY["+item+"]",    
		item,   
		TEST_ARRAY[item] );
}

new TestCase( SECTION,
	      "new Array( ["+TEST_LENGTH+" arguments] ) +''", 
	      ARGUMENTS,         
	      TEST_ARRAY +"" );

new TestCase( SECTION,
	      "TEST_ARRAY.toString",                          
	      Array.prototype.toString,  
	      TEST_ARRAY.toString );

new TestCase( SECTION,
	      "TEST_ARRAY.join",                              
	      Array.prototype.join,      
	      TEST_ARRAY.join );

new TestCase( SECTION,
	      "TEST_ARRAY.sort",                              
	      Array.prototype.sort,      
	      TEST_ARRAY.sort );

new TestCase( SECTION,
	      "TEST_ARRAY.reverse",                           
	      Array.prototype.reverse,   
	      TEST_ARRAY.reverse );

new TestCase( SECTION,
	      "TEST_ARRAY.length",                            
	      TEST_LENGTH,       
	      TEST_ARRAY.length );

new TestCase( SECTION,
	      "TEST_ARRAY.toString = Object.prototype.toString; TEST_ARRAY.toString()",
	      "[object Array]",
	      eval("TEST_ARRAY.toString = Object.prototype.toString; TEST_ARRAY.toString()") );

test();
