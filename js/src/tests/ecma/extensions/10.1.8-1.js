





































gTestfile = '10.1.8-1.js';





































var SECTION = "10.1.8";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Arguments Object";

writeHeaderToLog( SECTION + " "+ TITLE);

var ARG_STRING = "value of the argument property";

new TestCase( SECTION,
	      "GetCallee()",
	      GetCallee,
	      GetCallee() );

var LIMIT = 100;

for ( var i = 0, args = "" ; i < LIMIT; i++ ) {
  args += String(i) + ( i+1 < LIMIT ? "," : "" );

}

var LENGTH = eval( "GetLength("+ args +")" );

new TestCase( SECTION,
	      "GetLength("+args+")",
	      100,
	      LENGTH );

var ARGUMENTS = eval( "GetArguments( " +args+")" );

for ( var i = 0; i < 100; i++ ) {
  new TestCase( SECTION,
		"GetArguments("+args+")["+i+"]",
		i,
		ARGUMENTS[i] );
}

test();

function TestFunction() {
  var arg_proto = arguments.__proto__;
}
function GetCallee() {
  var c = arguments.callee;
  return c;
}
function GetArguments() {
  var a = arguments;
  return a;
}
function GetLength() {
  var l = arguments.length;
  return l;
}

function AnotherTestFunction() {
  this.__proto__ = new Prototype();
  return this;
}
