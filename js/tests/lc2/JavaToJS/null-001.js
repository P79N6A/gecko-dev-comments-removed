





















































var SECTION = "LiveConnect";
var VERSION = "1_3";
var TITLE   = "Java null to JavaScript Object";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);


var choice = new java.awt.Choice();

testcases[testcases.length] = new TestCase(
    SECTION,
    "var choice = new java.awt.Choice(); choice.getSelectedObjects()",
    null,
    choice.getSelectedObjects() );

testcases[testcases.length] = new TestCase(
    SECTION,
    "typeof choice.getSelectedObjects()",
    "object",
    typeof choice.getSelectedObjects() );



test();

function CheckType( et, at ) {
}
function CheckValue( ev, av ) {
}
