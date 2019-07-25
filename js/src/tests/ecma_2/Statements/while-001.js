
















var SECTION = "while-001";
var VERSION = "ECMA_2";
var TITLE   = "while statement";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

DoWhile();
test();

function DoWhile() {
  result = "pass";

  while (false) {
    result = "fail";
    break;
  }

  new TestCase(
    SECTION,
    "while statement: don't evaluate statement is expression is false",
    "pass",
    result );

}
