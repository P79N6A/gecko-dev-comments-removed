





































gTestfile = '10.2.2-1.js';


























var SECTION = "10.2.2-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Eval Code";

writeHeaderToLog( SECTION + " "+ TITLE);

var THIS = eval("this");

new TestCase( SECTION,
	      "this +''",
	      GLOBAL,
	      THIS + "" );

var GLOBAL_PROPERTIES = new Array();
var i = 0;

for ( p in THIS ) {
  GLOBAL_PROPERTIES[i++] = p;
}

for ( i = 0; i < GLOBAL_PROPERTIES.length; i++ ) {
  new TestCase( SECTION,
		GLOBAL_PROPERTIES[i] +" == THIS["+GLOBAL_PROPERTIES[i]+"]",
		true,
		eval(GLOBAL_PROPERTIES[i]) == eval( "THIS[GLOBAL_PROPERTIES[i]]") );
}



var RESULT = THIS == this;

new TestCase( SECTION,
	      "eval( 'this == THIS' )",
	      true,
	      RESULT );

var RESULT = THIS +'';

new TestCase( SECTION,
	      "eval( 'this + \"\"' )",
	      GLOBAL,
	      RESULT );


new TestCase( SECTION,
	      "eval( 'this == THIS' )",
	      true,
	      eval( "this == THIS" ) );

new TestCase( SECTION,
	      "eval( 'this + \"\"' )",
	      GLOBAL,
	      eval( "this +''") );


test();
