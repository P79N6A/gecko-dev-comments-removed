








































gTestfile = 'ToDouble-001.js';


















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
  "dt.setDouble( bool )",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  1,
  "number");

bool = new Boolean(false);

a[i++] = new TestObject(
  "dt.setDouble( bool )",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  0,
  "number");

var number = new Number(0);

a[i++] = new TestObject(
  "dt.setDouble( number )",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  0,
  "number");

nan = new Number(NaN);

a[i++] = new TestObject(
  "dt.setDouble( nan )",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  NaN,
  "number");

infinity = new Number(Infinity);

a[i++] = new TestObject(
  "dt.setDouble( infinity )",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  Infinity,
  "number");

var neg_infinity = new Number(-Infinity);

a[i++] = new TestObject(
  "dt.setDouble( new Number(neg_infinity))",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  -Infinity,
  "number");

var string  = new String("JavaScript String Value");

a[i++] = new TestObject(
  "dt.setDouble(string)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  NaN,
  "number");

var string  = new String("1234567890");

a[i++] = new TestObject(
  "dt.setDouble(string)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  1234567890,
  "number");


var string  = new String("1234567890.123456789");

a[i++] = new TestObject(
  "dt.setDouble(string)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  1234567890.123456789,
  "number");

var myobject = new MyObject( "9876543210" );

a[i++] = new TestObject(
  "dt.setDouble( myobject )",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  9876543210,
  "number");

myobject = new MyOtherObject( "5551212");

a[i++] = new TestObject(
  "dt.setDouble( myobject )",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  5551212,
  "number");

myobject = new AnotherObject( "6060842");

a[i++] = new TestObject(
  "dt.setDouble( myobject )",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  6060842,
  "number");

var object = new Object();

a[i++] = new TestObject(
  "dt.setDouble( object )",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  NaN,
  "number");

a[i++] = new TestObject(
  "dt.setDouble( MyObject )",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  NaN,
  "number");

a[i++] = new TestObject(
  "dt.setDouble( this )",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  NaN,
  "number");

a[i++] = new TestObject(
  "dt.setDouble( Math )",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  NaN,
  "number");

a[i++] = new TestObject(
  "dt.setDouble( Function )",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  NaN,
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
