





































gTestfile = 'delete-001.js';













var SECTION = "JS1_2";
var VERSION = "JS1_2";
var TITLE   = "The variable statement";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);





for ( p in this ) {
  delete p;
}

var result ="";

for ( p in this ) {
  result += String( p );
}



new TestCase( SECTION,
	      "delete all properties of the global object",
	      "PASSED",
	      result == "" ? "FAILED" : "PASSED" );


test();

