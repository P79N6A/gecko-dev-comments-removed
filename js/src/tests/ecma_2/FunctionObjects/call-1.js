














var SECTION = "call-1";
var VERSION = "ECMA_2";
var TITLE   = "Function.prototype.call";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "ConvertToString.call(this, this)",
	      GLOBAL,
	      ConvertToString.call(this, this));

new TestCase( SECTION,
	      "ConvertToString.call(Boolean, Boolean.prototype)",
	      "false",
	      ConvertToString.call(Boolean, Boolean.prototype));

new TestCase( SECTION,
	      "ConvertToString.call(Boolean, Boolean.prototype.valueOf())",
	      "false",
	      ConvertToString.call(Boolean, Boolean.prototype.valueOf()));

test();

function ConvertToString(obj) {
  return obj +"";
}
