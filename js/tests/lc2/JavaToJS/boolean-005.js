





































gTestfile = 'boolean-005.js';










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


java_array[i] = new JavaValue(  java.lang.Boolean.FALSE  );
test_array[i] = new TestValue(  "java.lang.Boolean.FALSE",
				false );

i++;


java_array[i] = new JavaValue(  java.lang.Boolean.TRUE  );
test_array[i] = new TestValue(  "java.lang.Boolean.TRUE",
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

  
  new TestCase( SECTION,
		"("+ testval.description+") + ''",
		testval.string,
		javaval.string );
}
function JavaValue( value ) {
  
  this.javaclass = value.getClass();







  this.string = value + "";
  print( this.string );
  this.value  = value.booleanValue();
  this.type   = typeof value;

  return this;
}
function TestValue( description, value ) {
  this.description = description;
  this.string = String( value );
  this.value = value;
  this.type =  E_TYPE;
  this.javaclass = E_JAVACLASS;
  this.jsclass = E_JSCLASS;
  return this;
}
