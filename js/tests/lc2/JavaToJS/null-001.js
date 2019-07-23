





















































var SECTION = "LiveConnect";
var VERSION = "1_3";
var TITLE   = "Java null to JavaScript Object";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);


var hashMap = new java.util.HashMap();

testcases[testcases.length] = new TestCase(
    SECTION,
    "var hashMap = new java.util.HashMap(); hashMap.get('unknown');",
    null,
    hashMap.get("unknown") );

testcases[testcases.length] = new TestCase(
    SECTION,
    "typeof hashMap.get('unknown')",
    "object",
    typeof hashMap.get('unknown') );



test();

function CheckType( et, at ) {
}
function CheckValue( ev, av ) {
}
