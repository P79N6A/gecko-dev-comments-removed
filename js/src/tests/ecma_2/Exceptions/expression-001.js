






















var SECTION = "expression-001";
var VERSION = "JS1_4";
var TITLE   = "Conditional operator ( ? : )"
  startTest();
writeHeaderToLog( SECTION + " " + TITLE );



var result = "Failed"
  var exception = "No exception was thrown";

try {
  eval("var MY_VAR = true ? \"EXPR1\", \"EXPR2\" : \"EXPR3\"");
} catch ( e ) {
  result = "Passed";
  exception = e.toString();
}

new TestCase(
  SECTION,
  "comma expression in a conditional statement "+
  "(threw "+ exception +")",
  "Passed",
  result );


test();
