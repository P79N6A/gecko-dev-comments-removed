












































































var SECTION = "LiveConnect";
var VERSION = "1_3";
var TITLE   = "LiveConnect JavaScript to Java Data Type Conversion";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);


var E_TYPE = "object";


var E_JSCLASS = "[object JavaObject]";

var a = new Array();
var i = 0;

a[i++] = new TestObject( "java.lang.Float.toString(0)",
			 java.lang.Float.toString(0), "0.0" );

a[i++] = new TestObject( "java.lang.Float.toString(NaN)",
			 java.lang.Float.toString(NaN), "NaN" );

a[i++] = new TestObject( "java.lang.Float.toString(5)",
			 java.lang.Float.toString(5), "5.0" );

a[i++] = new TestObject( "java.lang.Float.toString(9.9)",
			 java.lang.Float.toString(9.9), "9.9" );

a[i++] = new TestObject( "java.lang.Float.toString(-9.9)",
			 java.lang.Float.toString(-9.9), "-9.9" );

for ( var i = 0; i < a.length; i++ ) {

    
    new TestCase(
	SECTION,
	"typeof (" + a[i].description +")",
	a[i].type,
	typeof a[i].javavalue );








    
    new TestCase(
	SECTION,
	"String(" + a[i].description +")",
	a[i].jsvalue,
	String( a[i].javavalue ) );
}

test();

function TestObject( description, javavalue, jsvalue ) {
    this.description = description;
    this.javavalue = javavalue;
    this.jsvalue = jsvalue;
    this.type = E_TYPE;



    return this;
}
