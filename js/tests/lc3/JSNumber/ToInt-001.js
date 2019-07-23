

























































var SECTION = "number conversion to int";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
SECTION;
var BUGNUMBER="335589";

startTest();

var dt = new DT();

var a = new Array();
var i = 0;









a[i++] = new TestObject(
    "dt.setInteger( 0 )",
    "dt.PUB_INT",
    "dt.getInteger()",
    "typeof dt.getInteger()",
    0,
    "number" );



a[i++] = new TestObject(
    "dt.setInteger( -0 )",
    "Infinity / dt.PUB_INT",
    "Infinity / dt.getInteger()",
    "typeof dt.getInteger()",
    Infinity,
    "number" );

a[i++] = new TestObject(
    "dt.setInteger( java.lang.Integer.MAX_VALUE )",
    "dt.PUB_INT",
    "dt.getInteger()",
    "typeof dt.getInteger()",
    java.lang.Integer.MAX_VALUE,
    "number" );

a[i++] = new TestObject(
    "dt.setInteger( java.lang.Integer.MIN_VALUE )",
    "dt.PUB_INT",
    "dt.getInteger()",
    "typeof dt.getInteger()",
    java.lang.Integer.MIN_VALUE,
    "number" );

a[i++] = new TestObject(
    "dt.setInteger( -java.lang.Integer.MAX_VALUE )",
    "dt.PUB_INT",
    "dt.getInteger()",
    "typeof dt.getInteger()",
    -java.lang.Integer.MAX_VALUE,
    "number" );

a[i++] = new TestObject(
    "dt.setInteger(1e-2000)",
    "dt.PUB_INT",
    "dt.getInteger()",
    "typeof dt.getInteger()",
    0,
    "number" );

a[i++] = new TestObject(
    "dt.setInteger(-1e-2000)",
    "dt.PUB_INT",
    "dt.getInteger()",
    "typeof dt.getInteger()",
    0,
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
	this.javaTypeValue = eval( javaType );

    this.jsValue   = jsValue;
    this.jsType      = jsType;
}
