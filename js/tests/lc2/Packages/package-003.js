















































var SECTION = "LiveConnect Packages";
var VERSION = "1_3";
var TITLE   = "LiveConnect Packages";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);


var E_TYPE = "object";


var E_JSCLASS = "[JavaPackage ";




var java_array = new Array();
var test_array = new Array();

var i = 0;

var js = Packages.javax.javascript;

java_array[i] = new JavaValue(  js  );
test_array[i] = new TestValue(  "javax.javascript" );
i++;

var util = java.util;

java_array[i] = new JavaValue(  util  );
test_array[i] = new TestValue(  "java.util" );
i++;

for ( i = 0; i < java_array.length; i++ ) {
    CompareValues( java_array[i], test_array[i] );

}

var v = new util.Vector();

test();

function CompareValues( javaval, testval ) {
    
    new TestCase( SECTION,
		  "typeof (" + testval.description +")",
		  testval.type,
		  javaval.type );

    
    new TestCase( SECTION,
		  "(" + testval.description +").getJSClass()",
		  testval.jsclass,
		  javaval.jsclass );

    
    new TestCase( SECTION,
		  "Number (" + testval.description +")",
		  NaN,
		  Number( javaval.value ) );

    
    new TestCase( SECTION,
		  "String (" + testval.description +")",
		  testval.jsclass,
		  String(javaval.value) );







	
	new TestCase( SECTION,
		      "Boolean (" + testval.description +")",
		      true,
		      Boolean( javaval.value ) );
	
	new TestCase( SECTION,
		      "(" + testval.description +") +0",
		      testval.jsclass +"0",
		      javaval.value + 0);
}
function JavaValue( value ) {
    this.value  = value;
    this.type   = typeof value;
    this.jsclass = value +""
	return this;
}
function TestValue( description ) {
    this.packagename = (description.substring(0, "Packages.".length) ==
			"Packages.") ? description.substring("Packages.".length, description.length ) :
        description;

    this.description = description;
    this.type =  E_TYPE;
    this.jsclass = E_JSCLASS +  this.packagename +"]";
    return this;
}
