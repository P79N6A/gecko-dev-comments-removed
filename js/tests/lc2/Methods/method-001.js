
















































var SECTION = "LiveConnect Objects";
var VERSION = "1_3";
var TITLE   = "Calling Static Methods";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);



var E_TYPE = "object";


var E_JSCLASS = "[object JavaObject]";




var java_array = new Array();
var test_array = new Array();

var i = 0;

java_array[i] = new JavaValue(  java.lang.String.valueOf(true)  );
test_array[i] = new TestValue(  "java.lang.String.valueOf(true)",
				"object", "java.lang.String", "true" );

i++;

java_array[i] = new JavaValue( java.awt.Color.getHSBColor(0.0, 0.0, 0.0) );
test_array[i] = new TestValue( "java.awt.Color.getHSBColor(0.0, 0.0, 0.0)",
			       "object", "java.awt.Color", "java.awt.Color[r=0,g=0,b=0]" );

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
		  "("+testval.description +").getClass().equals( " +
		  "java.lang.Class.forName( '" + testval.classname +
		  "' ) )",
		  true,
		  (javaval.javaclass).equals( testval.javaclass ) );
    
    new TestCase(
        SECTION,
        "("+testval.description+") +''",
        testval.stringval,
        javaval.value +"" );
}
function JavaValue( value ) {
    this.type   = typeof value;
    this.value = value;




    this.javaclass = value.getClass();

    return this;
}
function TestValue( description, type, classname, stringval ) {
    this.description = description;
    this.type =  type;
    this.classname = classname;
    this.javaclass = java.lang.Class.forName( classname );
    this.stringval = stringval;

    return this;
}
