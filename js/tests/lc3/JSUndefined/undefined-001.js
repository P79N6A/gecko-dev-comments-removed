








































gTestfile = 'undefined-001.js';


















var SECTION = "undefined conversion";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
  SECTION;
startTest();

var dt = new DT();

var a = new Array();
var i = 0;




a[i++] = new TestObject(
  "dt.setObject( undefined )",
  "dt.PUB_OBJECT +''",
  "dt.getObject() + ''",
  "dt.getObject().getClass()",
  "undefined",
  java.lang.Class.forName( "java.lang.String" ) );




a[i++] = new TestObject(
  "dt.setStringObject( undefined )",
  "dt.PUB_STRING +''",
  "dt.getStringObject() + ''",
  "dt.getStringObject().getClass()",
  "undefined",
  java.lang.Class.forName( "java.lang.String" ) );

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
