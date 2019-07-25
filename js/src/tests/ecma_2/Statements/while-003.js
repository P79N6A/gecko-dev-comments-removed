















var SECTION = "while-003";
var VERSION = "ECMA_2";
var TITLE   = "while statement";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

DoWhile( new DoWhileObject(
	   "while expression is true",
	   true,
	   "result = \"pass\";" ));

DoWhile( new DoWhileObject(
	   "while expression is 1",
	   1,
	   "result = \"pass\";" ));

DoWhile( new DoWhileObject(
	   "while expression is new Boolean(false)",
	   new Boolean(false),
	   "result = \"pass\";" ));

DoWhile( new DoWhileObject(
	   "while expression is new Object()",
	   new Object(),
	   "result = \"pass\";" ));

DoWhile( new DoWhileObject(
	   "while expression is \"hi\"",
	   "hi",
	   "result = \"pass\";" ));







test();

function DoWhileObject( d, e, s ) {
  this.description = d;
  this.whileExpression = e;
  this.statements = s;
}

function DoWhile( object ) {
  result = "fail:  statements in while block were not evaluated";

  while ( expression = object.whileExpression ) {
    eval( object.statements );
    break;
  }

  

  new TestCase(
    SECTION,
    "verify that while expression was evaluated (should be "+
    object.whileExpression +")",
    "pass",
    (object.whileExpression == expression ||
     ( isNaN(object.whileExpression) && isNaN(expression) )
      ) ? "pass" : "fail" );

  new TestCase(
    SECTION,
    object.description,
    "pass",
    result );
}
