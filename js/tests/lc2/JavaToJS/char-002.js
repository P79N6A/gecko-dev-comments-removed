


























































var SECTION = "LiveConnect";
var VERSION = "1_3";
var TITLE   = "Java char return value to JavaScript Object";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);


var E_TYPE = "number";




var java_array = new Array();
var test_array = new Array();

var i = 0;



var os = java.lang.System.getProperty( "os.name" );
var v;

if ( os.startsWith( "Windows" ) ) {
    v = 92;
} else {
    if ( os.startsWith( "Mac" ) ) {
	v = 58;
    } else {
	v = 47;
    }
}

java_array[i] = new JavaValue(  java.io.File.separatorChar   );
test_array[i] = new TestValue(  "java.io.File.separatorChar", v );

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
