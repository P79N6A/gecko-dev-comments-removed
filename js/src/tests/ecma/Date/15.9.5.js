




























var SECTION = "15.9.5";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Properties of the Date Prototype Object";

writeHeaderToLog( SECTION + " "+ TITLE);


Date.prototype.getClass = Object.prototype.toString;

new TestCase( SECTION,
	      "Date.prototype.getClass",
	      "[object Object]",
	      Date.prototype.getClass() );
new TestCase( SECTION,
	      "Date.prototype.valueOf()",
	      "TypeError",
	      (function() { try { Date.prototype.valueOf() } catch (e) { return e.constructor.name; } })());
new TestCase( SECTION,
          "Date.prototype.__proto__ == Object.prototype",
          true,
          Date.prototype.__proto__ == Object.prototype );
test();
