





































gTestfile = 'array-004.js';










var SECTION = "LiveConnect";
var VERSION = "1_3";
var TITLE   = "Java Array to JavaScript JavaArray object";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);




var E_TYPE = "object";
var E_CLASS = "[object JavaArray]";




var java_array = new Array();
var test_array = new Array();

var i = 0;



var byte_array = ( new java.lang.String("ABCDEFGHIJKLMNOPQRSTUVWXYZ") ).getBytes();

java_array[i] = new JavaValue( byte_array );
test_array[i] = new TestValue( "( new java.lang.String('ABCDEFGHIJKLMNOPQRSTUVWXYZ') ).getBytes()",
			       "ABCDEFGHIJKLMNOPQRSTUVWXYZ".length
  );
i++;



var char_array = ( new java.lang.String("rhino") ).toCharArray();

java_array[i] = new JavaValue( char_array );
test_array[i] = new TestValue( "( new java.lang.String('rhino') ).toCharArray()",
			       "rhino".length );
i++;


for ( i = 0; i < java_array.length; i++ ) {
  CompareValues( java_array[i], test_array[i] );
}

test();

function CompareValues( javaval, testval ) {
  
  new TestCase( SECTION,
		"("+ testval.description +").length",
		testval.value,
		javaval.length );
  
  new TestCase(
    SECTION,
    "("+testval.description+")[-1]",
    void 0,
    javaval[-1] );

  
  new TestCase(
    SECTION,
    "("+testval.description+")["+testval.value+"]",
    void 0,
    javaval[testval.value] );


}
function JavaValue( value ) {
  this.value  = value;
  this.length = value.length;
  this.type   = typeof value;
  this.classname = this.value.toString();

  return this;
}
function TestValue( description, value ) {
  this.description = description;
  this.length = value
    this.value = value;
  this.type =  E_TYPE;
  this.classname = E_CLASS;
  return this;
}
