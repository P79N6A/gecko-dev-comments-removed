






































gTestfile = '10.1.8-3.js';
















var SECTION = "10.1.8-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Arguments Object";
writeHeaderToLog( SECTION + " "+ TITLE);

var expected = "object";
var actual = (function () { return typeof arguments; })();
reportCompare(expected, actual, "typeof arguments == object");

