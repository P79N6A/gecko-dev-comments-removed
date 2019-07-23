





































gTestfile = 'number-002.js';






























var SECTION = "LiveConnect";
var VERSION = "1_3";
var TITLE   = "Java Number Primitive to JavaScript Object";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);




var E_TYPE = "number";




var java_array = new Array();
var test_array = new Array();

var i = 0;



java_array[i] = new JavaValue(  java.lang.Byte.MIN_VALUE );
test_array[i] = new TestValue(  "java.lang.Byte.MIN_VALUE",
				-128 )
  i++;


java_array[i] = new JavaValue(  java.lang.Short.MIN_VALUE );
test_array[i] = new TestValue(  "java.lang.Short.MIN_VALUE",
				-32768 )
  i++;



java_array[i] = new JavaValue( java.lang.Integer.MIN_VALUE );
test_array[i] = new TestValue( "java.lang.Integer.MIN_VALUE",
			       -2147483648 )
  i++;




var java_rect = new java.awt.Rectangle( 1,2,3,4 );

java_array[i] = new JavaValue( java_rect.width );
test_array[i] = new TestValue( "java_object = new java.awt.Rectangle( 1,2,3,4 ); java_object.width",
			       3 );
i++;


java_array[i] = new JavaValue(  java.lang.Long.MIN_VALUE );
test_array[i] = new TestValue(  "java.lang.Long.MIN_VALUE",
				-9223372036854776000 );
i++;


java_array[i] = new JavaValue(  java.lang.Float.MAX_VALUE );
test_array[i] = new TestValue(  "java.lang.Float.MAX_VALUE",
				3.4028234663852886e+38 )
  i++;


java_array[i] = new JavaValue(  java.lang.Double.MAX_VALUE );
test_array[i] = new TestValue(  "java.lang.Double.MAX_VALUE",
				1.7976931348623157e+308 )
  i++;


java_array[i] = new JavaValue(  java.lang.Character.MAX_VALUE );
test_array[i] = new TestValue(  "java.lang.Character.MAX_VALUE",
				65535 );
i++;

for ( i = 0; i < java_array.length; i++ ) {
  CompareValues( java_array[i], test_array[i] );

}

test();
function CompareValues( javaval, testval ) {
  
  new TestCase( SECTION,
		testval.description,
		testval.value,
		javaval.value );
  

  new TestCase( SECTION,
		"typeof (" + testval.description +")",
		testval.type,
		javaval.type );
}
function JavaValue( value ) {
  this.value  = value.valueOf();
  this.type   = typeof value;
  return this;
}
function TestValue( description, value, type  ) {
  this.description = description;
  this.value = value;
  this.type =  E_TYPE;

  return this;
}
