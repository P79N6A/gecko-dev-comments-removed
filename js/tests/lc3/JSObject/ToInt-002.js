

























































var SECTION = "JavaScript Object to int";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
SECTION;
startTest();

var dt = new DT();

var a = new Array();
var i = 0;

function MyFunction() {
    return "hello";
}
MyFunction.valueOf = new Function( "return 999" );

function MyOtherFunction() {
    return "goodbye";
}
MyOtherFunction.valueOf = null;
MyOtherFunction.toString = new Function( "return 999" );








a[i++] = new TestObject(
    "dt.setInteger( MyFunction )",
    "dt.PUB_INT",
    "dt.getInteger()",
    "typeof dt.getInteger()",
    999,
    "number");

a[i++] = new TestObject(
    "dt.setInteger( MyOtherFunction )",
    "dt.PUB_INT",
    "dt.getInteger()",
    "typeof dt.getInteger()",
    999,
    "number");

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
