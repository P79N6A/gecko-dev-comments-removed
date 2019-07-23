





































gTestfile = 'enum-001.js';















var SECTION = "enum-001";
var VERSION = "JS1_3";
var TITLE   = "The variable statement";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var v = new java.util.Vector();
v.addElement("PASSED!");
v.addElement("PASSED!");
v.addElement("PASSED!");

for (e = v.elements(), result = new Array(), i = 0 ; e.hasMoreElements();
     i++ )
{
  result[i] = String( e.nextElement() );
}

for ( i = 0; i < result.length; i++ ) {
  new TestCase( SECTION,
		"test enumeration of a java object:  element at " + i,
		"PASSED!",
		result[i] );
}

test();

