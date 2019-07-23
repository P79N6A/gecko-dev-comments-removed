












































var SECTION = "LiveConnect Classes";
var VERSION = "1_3";
var TITLE   = "JavaClass objects";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);




var E_TYPE = "function";


var E_JSCLASS = "[object JavaClass]";




var java_array = new Array();
var test_array = new Array();

var i = 0;

java_array[i] = new JavaValue(  java.awt.Image  );
test_array[i] = new TestValue(  "java.awt.Image" );
i++;

java_array[i] = new JavaValue(  java.beans.Beans  );
test_array[i] = new TestValue(  "java.beans.Beans" );
i++;

java_array[i] = new JavaValue(  java.io.File  );
test_array[i] = new TestValue(  "java.io.File" );
i++;

java_array[i] = new JavaValue(  java.lang.String  );
test_array[i] = new TestValue(  "java.lang.String" );
i++;

java_array[i] = new JavaValue(  java.math.BigDecimal  );
test_array[i] = new TestValue(  "java.math.BigDecimal" );
i++;

java_array[i] = new JavaValue(  java.net.URL  );
test_array[i] = new TestValue(  "java.net.URL" );
i++;

java_array[i] = new JavaValue(  java.text.DateFormat  );
test_array[i] = new TestValue(  "java.text.DateFormat" );
i++;

java_array[i] = new JavaValue(  java.util.Vector  );
test_array[i] = new TestValue(  "java.util.Vector" );
i++;





for ( i = 0; i < java_array.length; i++ ) {
    CompareValues( java_array[i], test_array[i] );
}

test();
function CompareValues( javaval, testval ) {
    
    new TestCase( SECTION,
		  "typeof (" + testval.description +")",
		  testval.type,
		  javaval.type );







    
    new TestCase( SECTION,
		  "(" + testval.description +") +''",
		  testval.classname,
		  javaval.classname );
}
function JavaValue( value ) {
    



    this.classname = value +"";
    this.type   = typeof value;
    return this;
}
function TestValue( description, value ) {
    this.description = description;
    this.type =  E_TYPE;
    this.jclass = E_JSCLASS;
    this.lcclass = java.lang.Class.forName( description );

    this.classname = "[JavaClass " +
	(  ( description.substring(0,9) == "Packages." )
	   ? description.substring(9,description.length)
	   : description
	    ) + "]"


	return this;
}
