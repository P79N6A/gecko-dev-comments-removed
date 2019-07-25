













var SECTION = "";
var VERSION = "ECMA_2";
var TITLE   = "instanceof operator";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var b = new Boolean();

new TestCase( SECTION,
              "var b = new Boolean(); b instanceof Boolean",
              true,
              b instanceof Boolean );

new TestCase( SECTION,
              "b instanceof Object",
              true,
              b instanceof Object );

test();
