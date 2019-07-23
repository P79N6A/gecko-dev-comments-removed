





































gTestfile = 'object-002.js';



























var SECTION = "LiveConnect Objects";
var VERSION = "1_3";
var TITLE   = "Getting the Class of JavaObjects";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);



var E_TYPE = "object";


var E_JSCLASS = "[object JavaObject]";




var java_array = new Array();
var test_array = new Array();

var i = 0;

java_array[i] = new JavaValue(  new java.awt.Rectangle(1,2,3,4)  );
test_array[i] = new TestValue(  "new java.awt.Rectangle(1,2,3,4)", "java.awt.Rectangle" );
i++;

java_array[i] = new JavaValue(  new java.io.PrintStream( java.lang.System.out ) );
test_array[i] = new TestValue(  "new java.io.PrintStream(java.lang.System.out)", "java.io.PrintStream" );
i++;

java_array[i] = new JavaValue(  new java.lang.String("hello")  );
test_array[i] = new TestValue(  "new java.lang.String('hello')", "java.lang.String" );
i++;

java_array[i] = new JavaValue(  new java.net.URL("http://home.netscape.com/")  );
test_array[i] = new TestValue(  "new java.net.URL('http://home.netscape.com')", "java.net.URL" );
i++;









java_array[i] = new JavaValue(  new java.util.Vector()  );
test_array[i] = new TestValue(  "new java.util.Vector()", "java.util.Vector" );
i++;







for ( i = 0; i < java_array.length; i++ ) {
  CompareValues( java_array[i], test_array[i] );
}

test();

function CompareValues( javaval, testval ) {
  
  new TestCase( SECTION,
		"typeof (" + testval.description +" )",
		testval.type,
		javaval.type );







  
  new TestCase( SECTION,
		testval.description +".getClass().equals( " +
		"java.lang.Class.forName( '" + testval.classname +
		"' ) )",
		true,
		javaval.javaclass.equals( testval.javaclass ) );
}
function JavaValue( value ) {
  this.type   = typeof value;

  


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
