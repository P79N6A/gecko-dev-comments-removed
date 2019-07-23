





































gTestfile = 'constructor.js';










var SECTION = "wrapUnwrap.js";
var VERSION = "JS1_3";
var TITLE   = "LiveConnect";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var char_object = java.lang.Character.forDigit(22, 36);
test_typeof( "string", char_object+"a" );

var boolean_object = new java.lang.Boolean( true );
test_class( "java.lang.Boolean", boolean_object );

var boolean_object = new java.lang.Boolean( false );
test_class( "java.lang.Boolean", boolean_object );

var integer_object = new java.lang.Integer( 12345 );
test_class( "java.lang.Integer", integer_object );

var string_object = new java.lang.String( "string object value" );
test_class( "java.lang.String", string_object );


var float_object = new java.lang.Float( .009 * .009 );
test_class( "java.lang.Float", float_object );

var double_object = new java.lang.Double( java.lang.Math.PI );
test_class( "java.lang.Double", double_object );

var long_object = new java.lang.Long( 92233720368547760 );
test_class( "java.lang.Long", long_object );

var rect_object = new java.awt.Rectangle( 0, 0, 100, 100 );
test_class ( "java.awt.Rectangle", rect_object );

test();


function test_typeof( eType, someObject ) {
  new TestCase( SECTION,
		"typeof( " +someObject+")",
		eType,
		typeof someObject );
}




function test_class( eClass, javaObject ) {
  new TestCase( SECTION,
		javaObject +".getClass().equals( java.lang.Class.forName( " +
		eClass +")",
		true,
		(javaObject.getClass()).equals( java.lang.Class.forName(eClass)) );
}
