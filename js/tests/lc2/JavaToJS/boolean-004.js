





































gTestfile = 'boolean-004.js';




















var SECTION = "LiveConnect";
var VERSION = "1_3";
var TITLE   = "Java Boolean Object to JavaScript Object";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);



var E_TYPE = "object";



var E_JSCLASS = "[object JavaObject]";



var E_JAVACLASS = java.lang.Class.forName( "java.lang.Boolean" );




var java_array = new Array();
var test_array = new Array();

var i = 0;


java_array[i] = new JavaValue(  new java.lang.Boolean( true )  );
test_array[i] = new TestValue(  "new java.lang.Boolean( true )",
				true );

i++;


java_array[i] = new JavaValue(  new java.lang.Boolean( true )  );
test_array[i] = new TestValue(  "new java.lang.Boolean( true )",
				true );

i++;

for ( i = 0; i < java_array.length; i++ ) {
  CompareValues( java_array[i], test_array[i] );

}

test();

function CompareValues( javaval, testval ) {
  
  new TestCase( SECTION,
		"("+testval.description+").booleanValue()",
		testval.value,
		javaval.value );
  
  new TestCase( SECTION,
		"typeof (" + testval.description +")",
		testval.type,
		javaval.type );







  
  new TestCase( SECTION,
		"(" + testval.description +").getClass().equals( " + E_JAVACLASS +" )",
		true,
		javaval.javaclass.equals( testval.javaclass ) );
}
function JavaValue( value ) {
  
  this.javaclass = value.getClass();

  



  this.value  = value.booleanValue();
  this.type   = typeof value;
  return this;
}
function TestValue( description, value ) {
  this.description = description;
  this.value = value;
  this.type =  E_TYPE;
  this.javaclass = E_JAVACLASS;
  this.jsclass = E_JSCLASS;
  return this;
}
