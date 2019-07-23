





































gTestfile = 'instanceof-001.js';






var SECTION = "instanceof";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
  SECTION;
startTest();


new TestCase(
  "\"hi\" instance of java.lang.String",
  false,
  "hi" instanceof java.lang.String );

new TestCase(
  "new java.lang.String(\"hi\") instanceof java.lang.String",
  true,
  new java.lang.String("hi") instanceof java.lang.String );

new TestCase(
  "new java.lang.String(\"hi\") instanceof java.lang.Object",
  true,
  new java.lang.String("hi") instanceof java.lang.Object );

new TestCase(
  "java.lang.String instanceof java.lang.Class",
  false,
  java.lang.String instanceof java.lang.Class );

new TestCase(
  "java.lang.Class.forName(\"java.lang.String\") instanceof java.lang.Class",
  true,
  java.lang.Class.forName("java.lang.String") instanceof java.lang.Class );

new TestCase(
  "new java.lang.Double(5.0) instanceof java.lang.Double",
  true,
  new java.lang.Double(5.0) instanceof java.lang.Double );

new TestCase(
  "new java.lang.Double(5.0) instanceof java.lang.Number",
  true,
  new java.lang.Double(5.0) instanceof java.lang.Number );

new TestCase(
  "new java.lang.String(\"hi\").getBytes() instanceof java.lang.Double",
  true,
  new java.lang.Double(5.0) instanceof java.lang.Double );

test();
