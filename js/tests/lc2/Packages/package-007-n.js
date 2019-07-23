





































gTestfile = 'package-007-n.js';











var SECTION = "LiveConnect Packages";
var VERSION = "1_3";
var TITLE   = "LiveConnect Packages";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

java.util[0] ="hi";
java.util["x"] = "bye";

new TestCase( SECTION,
	      "java.util[0] = \"hi\"; typeof java.util[0]",
	      "undefined",
	      typeof java.util[0] );

test();

function CompareValues( javaval, testval ) {
  
  new TestCase( SECTION,
		"typeof (" + testval.description +")",
		testval.type,
		javaval.type );

  
  new TestCase( SECTION,
		"(" + testval.description +").getJSClass()",
		testval.jsclass,
		javaval.getJSClass() );

  
  new TestCase( SECTION,
		"Number (" + testval.description +")",
		NaN,
		Number( javaval ) );

  
  new TestCase( SECTION,
		"String (" + testval.description +")",
		testval.jsclass,
		String(javaval) );
  
  new TestCase( SECTION,
		"(" + testval.description +").toString()",
		testval.jsclass,
		(javaval).toString() );

  
  new TestCase( SECTION,
		"Boolean (" + testval.description +")",
		true,
		Boolean( javaval ) );
  
  new TestCase( SECTION,
		"(" + testval.description +") +0",
		testval.jsclass +"0",
		javaval + 0);
}
function JavaValue( value ) {
  this.value  = value;
  this.type   = typeof value;
  this.getJSClass = Object.prototype.toString;
  this.jsclass = value +""
    return this;
}
function TestValue( description ) {
  this.packagename = (description.substring(0, "Packages.".length) ==
		      "Packages.") ? description.substring("Packages.".length, description.length ) :
    description;

  this.description = description;
  this.type =  E_TYPE;
  this.jsclass = E_JSCLASS +  this.packagename +"]";
  return this;
}
