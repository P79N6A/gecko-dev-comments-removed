





































gTestfile = 'ToObject-001.js';






















var SECTION = "null";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
  SECTION;
startTest();

var dt = new DT();

var a = new Array();
var i = 0;






a[i++] = new TestObject(
  "dt.setBooleanObject( null )",
  "dt.PUB_BOOLEAN_OBJECT",
  "dt.getBooleanObject()",
  "typeof dt.getBooleanObject()",
  null,
  "object" );

a[i++] = new TestObject(
  "dt.setDoubleObject( null )",
  "dt.PUB_BOOLEAN_OBJECT",
  "dt.getDoubleObject()",
  "typeof dt.getDoubleObject()",
  null,
  "object" );

a[i++] = new TestObject(
  "dt.setStringObject( null )",
  "dt.PUB_STRING",
  "dt.getStringObject()",
  "typeof dt.getStringObject()",
  null,
  "object" );

for ( i = 0; i < a.length; i++ ) {
  new TestCase(
    a[i].description +"; "+ a[i].javaFieldName,
    a[i].jsValue,
    a[i].javaFieldValue );






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


  this.javaTypeName = javaType,
    this.javaTypeValue = typeof this.javaFieldValue;

  this.jsValue   = jsValue;
  this.jsType      = jsType;
}
