

























































var SECTION = "JavaObject to float";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
SECTION;
var BUGNUMBER = "335899";

startTest();

var dt = new DT();

var a = new Array();
var i = 0;







var newValue = Math.random();

a[i++] = new TestObject (
    "dt.PUB_DOUBLE_REPRESENTATION = java.lang.Double.MAX_VALUE;" +
    "dt.setFloat( dt )",
    "dt.PUB_FLOAT",
    "dt.getFloat()",
    "typeof dt.getFloat()",
    Infinity,
    "number" );

a[i++] = new TestObject (
    "dt.PUB_DOUBLE_REPRESENTATION = -java.lang.Double.MAX_VALUE;" +
    "dt.setFloat( dt )",
    "dt.PUB_FLOAT",
    "dt.getFloat()",
    "typeof dt.getFloat()",
    -Infinity,
    "number" );

a[i++] = new TestObject (
    "dt.PUB_DOUBLE_REPRESENTATION = java.lang.Float.MAX_VALUE * 2;" +
    "dt.setFloat( dt )",
    "dt.PUB_FLOAT",
    "dt.getFloat()",
    "typeof dt.getFloat()",
    Infinity,
    "number" );

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
	this.javaTypeValue = typeof this.javaFieldValue;

    this.jsValue   = jsValue;
    this.jsType      = jsType;
}
