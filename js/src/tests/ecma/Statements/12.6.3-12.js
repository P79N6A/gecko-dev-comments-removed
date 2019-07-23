





































gTestfile = '12.6.3-12.js';








































var SECTION = "12.6.3-12";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The for..in statement";

writeHeaderToLog( SECTION + " "+ TITLE);

var result = "PASSED";

for ( aVar in this ) {
  if (aVar == "aVar") {
    result = "FAILED"
      }
};

new TestCase(
  SECTION,
  "var result=''; for ( aVar in this ) { " +
  "if (aVar == 'aVar') {return a failure}; result",
  "PASSED",
  result );

test();

