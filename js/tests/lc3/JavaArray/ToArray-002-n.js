

























































var SECTION = "JavaArray to Object[]";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
SECTION;
startTest();

var dt = new DT();

var a = new Array();
var i = 0;






DESCRIPTION = "var vector = new java.util.Vector(); "+
"vector.addElement( \"a\" ); vector.addElement( \"b\" ); "+
"vector.copyInto( DT.PUB_STATIC_ARRAY_CHAR )";
EXPECTED = "error";

a[i++] = new TestObject(
    "var vector = new java.util.Vector(); "+
    "vector.addElement( \"a\" ); vector.addElement( \"b\" ); "+
    "vector.copyInto( DT.PUB_STATIC_ARRAY_CHAR )",
    "DT.PUB_STATIC_ARRAY_OBJECT[0] +''",
    "DT.staticGetObjectArray()[0] +''",
    "typeof DT.staticGetObjectArray()[0]",
    "error",
    "error" );

for ( i = 0; i < a.length; i++ ) {
    new TestCase(
	a[i].description +"; "+ a[i].javaFieldName,
	a[i].jsValue,
	a[i].javaFieldValue );

    new TestCase(
	a[i].description +"; " + a[i].javaMethodName,
	a[i].jsValue,
	a[i].javaMethodValue );

    new TestCase(
	a[i].javaTypeName,
	a[i].jsType,
	a[i].javaTypeValue );

}

test();

function TestObject( description, javaField, javaMethod, javaType,
		     jsValue, jsType )
{
    eval (description );

    this.description = description;
    this.javaFieldName = javaField;
    this.javaFieldValue = eval( javaField );
    this.javaMethodName = javaMethod;
    this.javaMethodValue = eval( javaMethod );
    this.javaTypeName = javaType,
	this.javaTypeValue = eval(javaType);

    this.jsValue   = jsValue;
    this.jsType      = jsType;
}
