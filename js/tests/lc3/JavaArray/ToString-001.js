
























































var SECTION = "JavaArray to String";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
SECTION;
startTest();

var dt = new DT();

var a = new Array();
var i = 0;








a[i++] = new TestObject (
    "dt.setStringObject( DT.staticGetByteArray() )",
    "dt.PUB_STRING +''",
    "dt.getStringObject() +''",
    "typeof dt.getStringObject() +''",
    DT.PUB_STATIC_ARRAY_BYTE +"",
    "string" );


a[i++] = new TestObject (
    "dt.setStringObject( DT.staticGetCharArray() )",
    "dt.PUB_STRING +''",
    "dt.getStringObject() +''",
    "typeof dt.getStringObject() +''",
    DT.PUB_STATIC_ARRAY_CHAR +"",
    "string" );

a[i++] = new TestObject (
    "dt.setStringObject( DT.staticGetShortArray() )",
    "dt.PUB_STRING +''",
    "dt.getStringObject() +''",
    "typeof dt.getStringObject() +''",
    DT.PUB_STATIC_ARRAY_SHORT +"",
    "string" );

a[i++] = new TestObject (
    "dt.setStringObject( DT.staticGetLongArray() )",
    "dt.PUB_STRING +''",
    "dt.getStringObject() +''",
    "typeof dt.getStringObject() +''",
    DT.PUB_STATIC_ARRAY_LONG +"",
    "string" );

a[i++] = new TestObject (
    "dt.setStringObject( DT.staticGetIntArray() )",
    "dt.PUB_STRING +''",
    "dt.getStringObject() +''",
    "typeof dt.getStringObject() +''",
    DT.PUB_STATIC_ARRAY_INT +"",
    "string" );

a[i++] = new TestObject (
    "dt.setStringObject( DT.staticGetFloatArray() )",
    "dt.PUB_STRING +''",
    "dt.getStringObject() +''",
    "typeof dt.getStringObject() +''",
    DT.PUB_STATIC_ARRAY_FLOAT +"",
    "string" );

a[i++] = new TestObject (
    "dt.setStringObject( DT.staticGetObjectArray() )",
    "dt.PUB_STRING +''",
    "dt.getStringObject() +''",
    "typeof dt.getStringObject() +''",
    DT.PUB_STATIC_ARRAY_OBJECT +"",
    "string" );

a[i++] = new TestObject (
    "dt.setStringObject(java.lang.String(new java.lang.String(\"hello\").getBytes()))",
    "dt.PUB_STRING +''",
    "dt.getStringObject() +''",
    "typeof dt.getStringObject() +''",
    "hello",
    "string" );

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
