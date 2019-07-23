





































gTestfile = '10.1.8-2.js';





































var SECTION = "10.1.8-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Arguments Object";

writeHeaderToLog( SECTION + " "+ TITLE);



var GetCallee       = new Function( "var c = arguments.callee; return c" );
var GetArguments    = new Function( "var a = arguments; return a" );
var GetLength       = new Function( "var l = arguments.length; return l" );

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
