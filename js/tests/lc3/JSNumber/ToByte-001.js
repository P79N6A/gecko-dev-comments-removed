


























































var SECTION = "number conversion to byte";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
SECTION;
startTest();

var dt = new DT();

var a = new Array();
var i = 0;









a[i++] = new TestObject(
    "dt.setByte( 0 )",
    "dt.PUB_BYTE",
    "dt.getByte()",
    "typeof dt.getByte()",
    0,
    "number" );



a[i++] = new TestObject(
    "dt.setByte( -0 )",
    "Infinity / dt.PUB_BYTE",
    "Infinity / dt.getByte()",
    "typeof dt.getByte()",
    Infinity,
    "number" );

a[i++] = new TestObject(
    "dt.setByte( java.lang.Byte.MAX_VALUE )",
    "dt.PUB_BYTE",
    "dt.getByte()",
    "typeof dt.getByte()",
    java.lang.Byte.MAX_VALUE,
    "number" );

a[i++] = new TestObject(
    "dt.setByte( java.lang.Byte.MIN_VALUE )",
    "dt.PUB_BYTE",
    "dt.getByte()",
    "typeof dt.getByte()",
    java.lang.Byte.MIN_VALUE,
    "number" );

a[i++] = new TestObject(
    "dt.setByte( -java.lang.Byte.MAX_VALUE )",
    "dt.PUB_BYTE",
    "dt.getByte()",
    "typeof dt.getByte()",
    -java.lang.Byte.MAX_VALUE,
    "number" );

a[i++] = new TestObject(
    "dt.setByte(1e-2000)",
    "dt.PUB_BYTE",
    "dt.getByte()",
    "typeof dt.getByte()",
    0,
    "number" );

a[i++] = new TestObject(
    "dt.setByte(-1e-2000)",
    "dt.PUB_BYTE",
    "dt.getByte()",
    "typeof dt.getByte()",
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
