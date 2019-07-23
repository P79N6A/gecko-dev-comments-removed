





































gTestfile = 'array-001.js';








var SECTION = "array-001";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0:  for ... in java objects";
SECTION;
startTest();








var dt = new Packages.com.netscape.javascript.qa.liveconnect.DataTypeClass;

var a = new Array();

a[a.length] = new TestObject(
  new java.lang.String("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789").getBytes(),
  "new java.lang.String(\"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\").getBytes()",
  36 );

a[a.length] = new TestObject(
  dt.PUB_ARRAY_SHORT,
  "dt.PUB_ARRAY_SHORT",
  dt.PUB_ARRAY_SHORT.length );

a[a.length] = new TestObject(
  dt.PUB_ARRAY_LONG,
  "dt.PUB_ARRAY_LONG",
  dt.PUB_ARRAY_LONG.length );

a[a.length] = new TestObject(
  dt.PUB_ARRAY_DOUBLE,
  "dt.PUB_ARRAY_DOUBLE",
  dt.PUB_ARRAY_DOUBLE.length );

a[a.length] = new TestObject(
  dt.PUB_ARRAY_BYTE,
  "dt.PUB_ARRAY_BYTE",
  dt.PUB_ARRAY_BYTE.length );

a[a.length] = new TestObject(
  dt.PUB_ARRAY_CHAR,
  "dt.PUB_ARRAY_CHAR",
  dt.PUB_ARRAY_CHAR.length );

a[a.length] = new TestObject(
  dt.PUB_ARRAY_OBJECT,
  "dt.PUB_ARRAY_OBJECT",
  dt.PUB_ARRAY_OBJECT.length );

for ( var i = 0; i < a.length; i++ ) {
  
  new TestCase(
    a[i].description +"; length",
    a[i].items,
    a[i].enumedArray.pCount );

  for ( var arrayItem = 0; arrayItem < a[i].items; arrayItem++ ) {
    new TestCase(
      "["+arrayItem+"]",
      a[i].javaArray[arrayItem],
      a[i].enumedArray[arrayItem] );
  }
}

test();

function TestObject( arr, descr, len ) {
  this.javaArray = arr;
  this.description = descr;
  this.items    = len;
  this.enumedArray = new enumObject(arr);
}

function enumObject( o ) {
  this.pCount = 0;
  for ( var p in o ) {
    this.pCount++;
    if ( !isNaN(p) ) {
      eval( "this["+p+"] = o["+p+"]" );
    } else {
      eval( "this." + p + " = o["+ p+"]" );
    }
  }
}

