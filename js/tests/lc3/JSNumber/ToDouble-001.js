





































gTestfile = 'ToDouble-001.js';























var SECTION = "number conversion";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
  SECTION;
startTest();

var dt = new DT();

var a = new Array();
var i = 0;







a[i++] = new TestObject(
  "dt.setDouble( 0 )",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  0,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble( -0 )",
  "Infinity / dt.PUB_DOUBLE",
  "Infinity / dt.getDouble()",
  "typeof dt.getDouble()",
  -Infinity,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble( Infinity )",
  "dt.PUB_DOUBLE ",
  "dt.getDouble() ",
  "typeof dt.getDouble()",
  Infinity,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble( -Infinity )",
  "dt.PUB_DOUBLE",
  "dt.getDouble() ",
  "typeof dt.getDouble()",
  -Infinity,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble( NaN )",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  NaN,
  "number" );



a[i++] = new TestObject(
  "dt.setDouble(077777777777777777)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  2251799813685247,
  "number" );


a[i++] = new TestObject(
  "dt.setDouble(077777777777777776)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  2251799813685246,
  "number" );


a[i++] = new TestObject(
  "dt.setDouble(0x1fffffffffffff)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  9007199254740991,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble(0x20000000000000)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  9007199254740992,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble(0x20123456789abc)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  9027215253084860,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble(0x20123456789abd)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  9027215253084860,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble(0x20123456789abe)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  9027215253084862,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble(0x20123456789abf)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  9027215253084864,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble(0x1000000000000080)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  1152921504606847000,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble(0x1000000000000081)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  1152921504606847200,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble(0x1000000000000100)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  1152921504606847200,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble(0x100000000000017f)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  1152921504606847200,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble(0x1000000000000180)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  1152921504606847500,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble(0x1000000000000181)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  1152921504606847500,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble(1.7976931348623157E+308)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  1.7976931348623157e+308,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble(1.7976931348623158e+308)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  1.7976931348623157e+308,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble(1.7976931348623159e+308)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  Infinity,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble(-1.7976931348623157E+308)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  -1.7976931348623157e+308,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble(-1.7976931348623158e+308)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  -1.7976931348623157e+308,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble(-1.7976931348623159e+308)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  -Infinity,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble(1e-2000)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  0,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble(1e2000)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  1e2000,
  "number" );

a[i++] = new TestObject(
  "dt.setDouble(-1e2000)",
  "dt.PUB_DOUBLE",
  "dt.getDouble()",
  "typeof dt.getDouble()",
  -1e2000,
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
