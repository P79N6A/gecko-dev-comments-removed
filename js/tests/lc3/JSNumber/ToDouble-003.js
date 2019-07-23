

























































var SECTION = "number conversion to float";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
SECTION;
startTest();

var dt = new DT();

var a = new Array();
var i = 0;







a[i++] = new TestObject(
    "dt.setFloat( 0 )",
    "dt.PUB_FLOAT",
    "dt.getFloat()",
    "typeof dt.getFloat()",
    0,
    "number" );

a[i++] = new TestObject(
    "dt.setFloat( -0 )",
    "Infinity / dt.PUB_FLOAT",
    "Infinity / dt.getFloat()",
    "typeof dt.getFloat()",
    -Infinity,
    "number" );

a[i++] = new TestObject(
    "dt.setFloat( Infinity )",
    "dt.PUB_FLOAT ",
    "dt.getFloat() ",
    "typeof dt.getFloat()",
    Infinity,
    "number" );

a[i++] = new TestObject(
    "dt.setFloat( -Infinity )",
    "dt.PUB_FLOAT",
    "dt.getFloat() ",
    "typeof dt.getFloat()",
    -Infinity,
    "number" );

a[i++] = new TestObject(
    "dt.setFloat( NaN )",
    "dt.PUB_FLOAT",
    "dt.getFloat()",
    "typeof dt.getFloat()",
    NaN,
    "number" );

a[i++] = new TestObject(
    "dt.setFloat( java.lang.Float.MAX_VALUE )",
    "dt.PUB_FLOAT",
    "dt.getFloat()",
    "typeof dt.getFloat()",
    java.lang.Float.MAX_VALUE,
    "number" );

a[i++] = new TestObject(
    "dt.setFloat( java.lang.Float.MIN_VALUE )",
    "dt.PUB_FLOAT",
    "dt.getFloat()",
    "typeof dt.getFloat()",
    java.lang.Float.MIN_VALUE,
    "number" );

a[i++] = new TestObject(
    "dt.setFloat( -java.lang.Float.MAX_VALUE )",
    "dt.PUB_FLOAT",
    "dt.getFloat()",
    "typeof dt.getFloat()",
    -java.lang.Float.MAX_VALUE,
    "number" );

a[i++] = new TestObject(
    "dt.setFloat( -java.lang.Float.MIN_VALUE )",
    "dt.PUB_FLOAT",
    "dt.getFloat()",
    "typeof dt.getFloat()",
    -java.lang.Float.MIN_VALUE,
    "number" );

a[i++] = new TestObject(
    "dt.setFloat(1.7976931348623157E+308)",
    "dt.PUB_FLOAT",
    "dt.getFloat()",
    "typeof dt.getFloat()",
    Infinity,
    "number" );

a[i++] = new TestObject(
    "dt.setFloat(1.7976931348623158e+308)",
    "dt.PUB_FLOAT",
    "dt.getFloat()",
    "typeof dt.getFloat()",
    Infinity,
    "number" );

a[i++] = new TestObject(
    "dt.setFloat(1.7976931348623159e+308)",
    "dt.PUB_FLOAT",
    "dt.getFloat()",
    "typeof dt.getFloat()",
    Infinity,
    "number" );

a[i++] = new TestObject(
    "dt.setFloat(-1.7976931348623157E+308)",
    "dt.PUB_FLOAT",
    "dt.getFloat()",
    "typeof dt.getFloat()",
    -Infinity,
    "number" );

a[i++] = new TestObject(
    "dt.setFloat(-1.7976931348623158e+308)",
    "dt.PUB_FLOAT",
    "dt.getFloat()",
    "typeof dt.getFloat()",
    -Infinity,
    "number" );

a[i++] = new TestObject(
    "dt.setFloat(-1.7976931348623159e+308)",
    "dt.PUB_FLOAT",
    "dt.getFloat()",
    "typeof dt.getFloat()",
    -Infinity,
    "number" );

a[i++] = new TestObject(
    "dt.setFloat(1e-2000)",
    "dt.PUB_FLOAT",
    "dt.getFloat()",
    "typeof dt.getFloat()",
    0,
    "number" );

a[i++] = new TestObject(
    "dt.setFloat(1e2000)",
    "dt.PUB_FLOAT",
    "dt.getFloat()",
    "typeof dt.getFloat()",
    Infinity,
    "number" );

a[i++] = new TestObject(
    "dt.setFloat(-1e2000)",
    "dt.PUB_FLOAT",
    "dt.getFloat()",
    "typeof dt.getFloat()",
    -Infinity,
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
