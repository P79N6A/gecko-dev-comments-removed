





































gTestfile = '11.13.1.js';




















var SECTION = "11.13.1";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Simple Assignment ( = )");

new TestCase( SECTION,   
              "SOMEVAR = true",    
              true,  
              SOMEVAR = true );

test();

