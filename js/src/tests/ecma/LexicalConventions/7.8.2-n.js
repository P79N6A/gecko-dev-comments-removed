





































gTestfile = '7.8.2-n.js';










var SECTION="7.8.2";
var VERSION="ECMA_1"
  startTest();
writeHeaderToLog(SECTION+" "+"Examples of Semicolon Insertion");




DESCRIPTION = "{ 1 2 } 3";
EXPECTED = "error";

new TestCase( "7.8.2",  "{ 1 2 } 3",         "error",   eval("{1 2 } 3")     );

test();
