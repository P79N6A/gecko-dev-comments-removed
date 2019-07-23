





































gTestfile = 'object-004.js';










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

new TestCase(
  SECTION,
  "vector.elementAt(0) +''",
  "hi",
  vector.elementAt(0)+"" );

new TestCase(
  SECTION,
  "vector.setElementAt( \"hello\", 0)",
  void 0,
  vector.setElementAt( "hello", 0) );

new TestCase(
  SECTION,
  "vector.elementAt(0) +''",
  "hello",
  vector.elementAt(0)+"" );

test();

