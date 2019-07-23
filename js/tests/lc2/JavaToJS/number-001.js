
































































var SECTION = "LiveConnect";
var VERSION = "1_3";
var TITLE   = "Java Number Primitive to JavaScript Object";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);



var E_TYPE = "number";




var java_array = new Array();
var test_array = new Array();

var i = 0;


java_array[i] = new JavaValue( java.lang.Integer.parseInt('255') );
test_array[i] = new TestValue( "java.lang.Integer.parseInt('255')",
			       255,
			       E_TYPE );

i++;

java_array[i] = new JavaValue( (new java.lang.Double( '123456789' )).intValue() );
test_array[i] = new TestValue( "(new java.lang.Double( '123456789' )).intValue()",
			       123456789,
			       E_TYPE );

i++;


java_array[i] = new JavaValue( (new java.lang.Integer( '123456789' )).doubleValue() );
test_array[i] = new TestValue( "(new java.lang.Integer( '123456789' )).doubleValue()",
			       123456789,
			       E_TYPE );


java_array[i] = new JavaValue( (new java.lang.Long('1234567891234567' )).longValue() );
test_array[i] = new TestValue( "(new java.lang.Long( '1234567891234567' )).longValue()",
			       1234567891234567,
			       E_TYPE );



java_array[i] = new JavaValue( (new java.lang.Float( '1.23456789' )).floatValue() );
test_array[i] = new TestValue( "(new java.lang.Float( '1.23456789' )).floatValue()",
			       1.23456789,
			       E_TYPE );

i++;


java_array[i] = new JavaValue(  (new java.lang.String("hello")).charAt(0) );
test_array[i] = new TestValue( "(new java.lang.String('hello')).charAt(0)",
			       "h".charCodeAt(0),
			       E_TYPE );
i++;


java_array[i] = new JavaValue(  (new java.lang.Byte(127)).shortValue() );
test_array[i] = new TestValue( "(new java.lang.Byte(127)).shortValue()",
			       127,
			       E_TYPE );
i++;


java_array[i] = new JavaValue( (new java.lang.Byte(127)).byteValue() );
test_array[i] = new TestValue( "(new java.lang.Byte(127)).byteValue()",
			       127,
			       E_TYPE );


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
function TestValue( description, value, type, classname ) {
    this.description = description;
    this.value = value;
    this.type =  type;
    return this;
}
