





































gTestfile = 'boolean-001.js';























var SECTION = "boolean conversion";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
  SECTION;
var BUGNUMBER = "335907";

startTest();

var dt = new DT();

var a = new Array();
var i = 0;




a[i++] = new TestObject(
  "dt.setBoolean( true )",
  "dt.PUB_BOOLEAN",
  "dt.getBoolean()",
  "typeof dt.getBoolean()",
  true,
  "boolean" );

a[i++] = new TestObject(
  "dt.setBoolean( false )",
  "dt.PUB_BOOLEAN",
  "dt.getBoolean()",
  "typeof dt.getBoolean()",
  false,
  "boolean" );




a[i++] = new TestObject(
  "dt.setBooleanObject( true )",
  "dt.PUB_BOOLEAN_OBJECT +''",
  "dt.getBooleanObject() +''",
  "dt.getBooleanObject().getClass()",
  "true",
  java.lang.Class.forName( "java.lang.Boolean") );

a[i++] = new TestObject(
  "dt.setBooleanObject( false )",
  "dt.PUB_BOOLEAN_OBJECT +''",
  "dt.getBooleanObject() +''",
  "dt.getBooleanObject().getClass()",
  "false",
  java.lang.Class.forName( "java.lang.Boolean") );





a[i++] = new TestObject(
  "dt.setObject( true )",
  "dt.PUB_OBJECT +''",
  "dt.getObject() +''",
  "dt.getObject().getClass()",
  "true",
  java.lang.Class.forName( "java.lang.Boolean") );

a[i++] = new TestObject(
  "dt.setObject( false )",
  "dt.PUB_OBJECT +''",
  "dt.getObject() +''",
  "dt.getObject().getClass()",
  "false",
  java.lang.Class.forName( "java.lang.Boolean") );





a[i++] = new TestObject(
  "dt.setStringObject( true )",
  "dt.PUB_STRING +''",
  "dt.getStringObject() +''",
  "dt.getStringObject().getClass()",
  "true",
  java.lang.Class.forName( "java.lang.String") );

a[i++] = new TestObject(
  "dt.setStringObject( false )",
  "dt.PUB_STRING +''",
  "dt.getStringObject() +''",
  "dt.getStringObject().getClass()",
  "false",
  java.lang.Class.forName( "java.lang.String") );

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
