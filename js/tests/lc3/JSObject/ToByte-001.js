








































gTestfile = 'ToByte-001.js';


















var SECTION = "JavaScript Object to java.lang.String";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
  SECTION;
startTest();

var dt = new DT();

var a = new Array();
var i = 0;
























var bool = new Boolean(true);

a[i++] = new TestObject(
  "dt.setByte( bool )",
  "dt.PUB_BYTE",
  "dt.getByte()",
  "typeof dt.getByte()",
  1,
  "number");

bool = new Boolean(false);

a[i++] = new TestObject(
  "dt.setByte( bool )",
  "dt.PUB_BYTE",
  "dt.getByte()",
  "typeof dt.getByte()",
  0,
  "number");

var number = new Number(0);

a[i++] = new TestObject(
  "dt.setByte( number )",
  "dt.PUB_BYTE",
  "dt.getByte()",
  "typeof dt.getByte()",
  0,
  "number");









































var string  = new String("127");

a[i++] = new TestObject(
  "dt.setByte(string)",
  "dt.PUB_BYTE",
  "dt.getByte()",
  "typeof dt.getByte()",
  127,
  "number");

var string  = new String("-128");

a[i++] = new TestObject(
  "dt.setByte(string)",
  "dt.PUB_BYTE",
  "dt.getByte()",
  "typeof dt.getByte()",
  -128,
  "number");

var myobject = new MyObject( "5.5" );

a[i++] = new TestObject(
  "dt.setByte( myobject )",
  "dt.PUB_BYTE",
  "dt.getByte()",
  "typeof dt.getByte()",
  5,
  "number");

myobject = new MyOtherObject( "-9.5");

a[i++] = new TestObject(
  "dt.setByte( myobject )",
  "dt.PUB_BYTE",
  "dt.getByte()",
  "typeof dt.getByte()",
  -9,
  "number");

myobject = new AnotherObject( "111");

a[i++] = new TestObject(
  "dt.setByte( myobject )",
  "dt.PUB_BYTE",
  "dt.getByte()",
  "typeof dt.getByte()",
  111,
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

function MyObject( stringValue ) {
  this.stringValue = String(stringValue);
  this.toString = new Function( "return this.stringValue" );
}

function MyOtherObject( value ) {
  this.toString = null;
  this.value = value;
  this.valueOf = new Function( "return this.value" );
}

function AnotherObject( value ) {
  this.toString = new Function( "return new Number(666)" );
  this.value = value;
  this.valueOf = new Function( "return this.value" );
}

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
