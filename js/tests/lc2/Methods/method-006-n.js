





































gTestfile = 'method-006-n.js';














var SECTION = "LiveConnect Objects";
var VERSION = "1_3";
var TITLE   = "Assigning a Non-Static Java Method to a JavaScript Object";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var java_string = new java.lang.String("LiveConnect");
var js_string   = "JavaScript";

DESCRIPTION = "var java_string = new java.lang.String(\"LiveConnect\");" +
  "var js_string = \"JavaScript\"" +
  "js_string.startsWith = java_string.startsWith"+
  "js_string.startsWith(\"J\")";

EXPECTED = "error";

js_string.startsWith = java_string.startsWith;

new TestCase(
  SECTION,
  "var java_string = new java.lang.String(\"LiveConnect\");" +
  "var js_string = \"JavaScript\"" +
  "js_string.startsWith = java_string.startsWith"+
  "js_string.startsWith(\"J\")",
  false,
  js_string.startsWith("J") );

test();

function MyObject() {
  this.println = java.lang.System.out.println;
  this.classForName = java.lang.Class.forName;
  return this;
}

