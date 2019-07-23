
























































var SECTION = "LiveConnect";
var VERSION = "1_3";
var TITLE   = "Java Boolean Primitive to JavaScript Object";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);



var E_TYPE = "boolean";




var java_array = new Array();
var test_array = new Array();

var i = 0;


java_array[i] = new JavaValue(  (new java.lang.Boolean(true)).booleanValue() );
test_array[i] = new TestValue(  "(new java.lang.Boolean(true)).booleanValue()",
				true )
    i++;


java_array[i] = new JavaValue(  (new java.lang.Boolean(false)).booleanValue() );
test_array[i] = new TestValue(  "(new java.lang.Boolean(false)).booleanValue()",
				false )
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
    this.value  = value;
    this.type   = typeof value;
    return this;
}
function TestValue( description, value ) {
    this.description = description;
    this.value = value;
    this.type =  E_TYPE;
    return this;
}
