















































var SECTION = "LiveConnect Objects";
var VERSION = "1_3";
var TITLE   = "Invoking Java Methods";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);



var E_TYPE = "object";


var E_JSCLASS = "[object JavaObject]";




var java_array = new Array();
var test_array = new Array();

var i = 0;



var rect = new java.awt.Rectangle(1,2,3,4);
var size = rect.getSize();

new TestCase(
    SECTION,
    "var size = (new java.awt.Rectangle(1,2,3,4)).getSize(); "+
    "size.getClass().equals(java.lang.Class.forName(\""+
    "java.awt.Dimension\"))",
    true,
    size.getClass().equals(java.lang.Class.forName("java.awt.Dimension")));

new TestCase(
    SECTION,
    "size.width",
    3,
    size.width );

new TestCase(
    SECTION,
    "size.height",
    4,
    size.height );


var r = rect.setSize(5,6);

new TestCase(
    SECTION,
    "var r = rect.setSize(5,6); r",
    void 0,
    r );



var string = new java.lang.String( "     hello     " );
s = string.trim()

    new TestCase(
        SECTION,
        "var string = new java.lang.String(\"     hello     \"); "+
        "var s = string.trim(); s.getClass().equals("+
        "java.lang.Class.forName(\"java.lang.String\")",
        true,
        s.getClass().equals(java.lang.Class.forName("java.lang.String")) );


new TestCase(
    SECTION,
    "s.length()",
    5,
    s.length() );

test();

function CompareValues( javaval, testval ) {
    
    new TestCase( SECTION,
		  "typeof (" + testval.description +" )",
		  testval.type,
		  javaval.type );

    
    new TestCase( SECTION,
		  "(" + testval.description +" ).getJSClass()",
		  testval.jsclass,
		  javaval.jsclass );
    
    new TestCase( SECTION,
		  testval.description +".getClass().equals( " +
		  "java.lang.Class.forName( '" + testval.classname +
		  "' ) )",
		  true,
		  javaval.javaclass.equals( testval.javaclass ) );
}
function JavaValue( value ) {
    this.type   = typeof value;
    
    value.__proto__.getJSClass = Object.prototype.toString;
    this.jsclass = value.getJSClass();
    this.javaclass = value.getClass();
    return this;
}
function TestValue( description, classname ) {
    this.description = description;
    this.classname = classname;
    this.type =  E_TYPE;
    this.jsclass = E_JSCLASS;
    this.javaclass = java.lang.Class.forName( classname );
    return this;
}
