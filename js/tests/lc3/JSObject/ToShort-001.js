








































gTestfile = 'ToShort-001.js';


















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
  "dt.setShort( bool )",
  "dt.PUB_SHORT",
  "dt.getShort()",
  "typeof dt.getShort()",
  1,
  "number");

bool = new Boolean(false);

a[i++] = new TestObject(
  "dt.setShort( bool )",
  "dt.PUB_SHORT",
  "dt.getShort()",
  "typeof dt.getShort()",
  0,
  "number");

var number = new Number(0);

a[i++] = new TestObject(
  "dt.setShort( number )",
  "dt.PUB_SHORT",
  "dt.getShort()",
  "typeof dt.getShort()",
  0,
  "number");

var string  = new String("32767");

a[i++] = new TestObject(
  "dt.setShort(string)",
  "dt.PUB_SHORT",
  "dt.getShort()",
  "typeof dt.getShort()",
  32767,
  "number");

var string  = new String("-32768");

a[i++] = new TestObject(
  "dt.setShort(string)",
  "dt.PUB_SHORT",
  "dt.getShort()",
  "typeof dt.getShort()",
  -32768,
  "number");

var myobject = new MyObject( "5.5" );

a[i++] = new TestObject(
  "dt.setShort( myobject )",
  "dt.PUB_SHORT",
  "dt.getShort()",
  "typeof dt.getShort()",
  5,
  "number");

myobject = new MyOtherObject( "-107.5");

a[i++] = new TestObject(
  "dt.setShort( myobject )",
  "dt.PUB_SHORT",
  "dt.getShort()",
  "typeof dt.getShort()",
  -107,
  "number");

myobject = new AnotherObject( "6666");

a[i++] = new TestObject(
  "dt.setShort( myobject )",
  "dt.PUB_SHORT",
  "dt.getShort()",
  "typeof dt.getShort()",
  6666,
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
