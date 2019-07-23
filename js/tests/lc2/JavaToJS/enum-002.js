





































gTestfile = 'enum-002.js';
















var SECTION = "enum-002";
var VERSION = "JS1_3";
var TITLE   = "The variable statement";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var v = new java.util.Vector();
v.addElement("TRUE");

for (e = v.elements(), result = new Array(), i = 0 ; e.hasMoreElements();
     i++ )
{
  result[i] = (new java.lang.Boolean(e.nextElement())).booleanValue();
}

for ( i = 0; i < result.length; i++ ) {
  new TestCase( SECTION,
		"test enumeration of a java object:  element at " + i,
		"true",
		String( result[i] ) );
}

test();

