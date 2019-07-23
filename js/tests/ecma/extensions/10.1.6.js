





































gTestfile = '10.1.6.js';

























var SECTION = "10.1.6";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Activation Object";

writeHeaderToLog( SECTION + " "+ TITLE);

var arguments = "FAILED!";

var ARG_STRING = "value of the argument property";

new TestCase( SECTION,
	      "(new TestObject(0,1,2,3,4,5)).length",
	      6,
	      (new TestObject(0,1,2,3,4,5)).length );

for ( i = 0; i < 6; i++ ) {

  new TestCase( SECTION,
		"(new TestObject(0,1,2,3,4,5))["+i+"]",
		i,
		(new TestObject(0,1,2,3,4,5))[i]);
}




new TestCase( SECTION,
	      "(new AnotherTestObject(1,2,3)).arguments",
	      ARG_STRING,
	      (new AnotherTestObject(1,2,3)).arguments );



new TestCase( SECTION,
	      "TestFunction(1,2,3)",
	      ARG_STRING,
	      TestFunction() + '' );


test();



function Prototype() {
  this.arguments = ARG_STRING;
}
function TestObject() {
  this.__proto__ = new Prototype();
  return arguments;
}
function AnotherTestObject() {
  this.__proto__ = new Prototype();
  return this;
}
function TestFunction() {
  arguments = ARG_STRING;
  return arguments;
}
function AnotherTestFunction() {
  this.__proto__ = new Prototype();
  return this;
}
