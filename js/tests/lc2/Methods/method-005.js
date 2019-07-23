
















































var SECTION = "LiveConnect Objects";
var VERSION = "1_3";
var TITLE   = "Assigning a Static Java Method to a JavaScript object";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var java_string = new java.lang.String("LiveConnect");
var js_string   = "JavaScript";

js_string.startsWith = java_string.startsWith;










var mo = new MyObject();

var c = mo.classForName( "java.lang.String" );

new TestCase(
    SECTION,
    "var mo = new MyObject(); "+
    "var c = mo.classForName(\"java.lang.String\");" +
    "c.equals(java.lang.Class.forName(\"java.lang.String\))",
    true,
    c.equals(java.lang.Class.forName("java.lang.String")) );



test();

function MyObject() {
    this.println = java.lang.System.out.println;
    this.classForName = java.lang.Class.forName;
    return this;
}

