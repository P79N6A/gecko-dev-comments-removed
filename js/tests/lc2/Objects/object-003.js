





































gTestfile = 'object-003.js';










var SECTION = "LiveConnect Objects";
var VERSION = "1_3";
var TITLE   = "Getting and setting JavaObject properties by index value";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var vector = new java.util.Vector();

new TestCase(
  SECTION,
  "var vector = new java.util.Vector(); vector.addElement(\"hi\")",
  void 0,
  vector.addElement("hi") );

test();


function EnumerateJavaObject( javaobject ) {
  var properties = new Array();
  for ( var p in javaobject ) {
    properties[properties.length] = new Property( p, javaobject[p] );
  }
  return properties;
}
function Property( name, value ) {
  this.name = name;
  this.value = value;
}

