






































gTestfile = 'JavaObjectFieldOrMethod-001.js';

var SECTION = "JavaObject Field or method access";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
  SECTION;
startTest();

var dt = new DT();

new TestCase(
  "dt.amIAFieldOrAMethod",
  String(dt.amIAFieldOrAMethod),
  "FIELD!" );

new TestCase(
  "dt.amIAFieldOrAMethod()",
  String(dt.amIAFieldOrAMethod()),
  "METHOD!" );

test();

