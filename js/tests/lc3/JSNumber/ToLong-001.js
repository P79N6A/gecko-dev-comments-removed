

























































var SECTION = "number conversion to long";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
SECTION;
startTest();

var dt = new DT();

var a = new Array();
var i = 0;









a[i++] = new TestObject(
    "dt.setLong( 0 )",
    "dt.PUB_LONG",
    "dt.getLong()",
    "typeof dt.getLong()",
    0,
    "number" );

a[i++] = new TestObject(
    "dt.setLong( -0 )",
    "Infinity / dt.PUB_LONG",
    "Infinity / dt.getLong()",
    "typeof dt.getLong()",
    Infinity,
    "number" );

a[i++] = new TestObject(
    "dt.setLong(1234567890123)",
    "dt.PUB_LONG",
    "dt.getLong()",
    "typeof dt.getLong()",
    1234567890123,
    "number" );

a[i++] = new TestObject(
    "dt.setLong(-1234567890123)",
    "dt.PUB_LONG",
    "dt.getLong()",
    "typeof dt.getLong()",
    -1234567890123,
    "number" );

a[i++] = new TestObject(
    "dt.setLong(0x7ffffffffffffff)",
    "dt.PUB_LONG",
    "dt.getLong()",
    "typeof dt.getLong()",
    0x7ffffffffffffff,
    "number" );

a[i++] = new TestObject(
    "dt.setLong(0x7ffffffffffffff)",
    "dt.PUB_LONG",
    "dt.getLong()",
    "typeof dt.getLong()",
    0x7ffffffffffffff,
    "number" );

a[i++] = new TestObject(
    "dt.setLong(0xfffffffffffffff)",
    "dt.PUB_LONG",
    "dt.getLong()",
    "typeof dt.getLong()",
    0xfffffffffffffff,
    "number" );

a[i++] = new TestObject(
    "dt.setLong(0x6fffffffffffffff)",
    "dt.PUB_LONG",
    "dt.getLong()",
    "typeof dt.getLong()",
    0x6fffffffffffffff,
    "number" );

a[i++] = new TestObject(
    "dt.setLong(-0x6fffffffffffffff)",
    "dt.PUB_LONG",
    "dt.getLong()",
    "typeof dt.getLong()",
    -0x6fffffffffffffff,
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
