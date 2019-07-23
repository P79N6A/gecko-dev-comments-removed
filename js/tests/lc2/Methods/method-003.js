





































gTestfile = 'method-003.js';












var SECTION = "LiveConnect Objects";
var VERSION = "1_3";
var TITLE   = "Type and Class of Java Methods";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);


var E_TYPE = "function";


var E_JSCLASS = "[object Function]";




var java_array = new Array();
var test_array = new Array();

var i = 0;

java_array[i] = new JavaValue(  java.lang.System.out.println  );
test_array[i] = new TestValue(  "java.lang.System.out.println" );
i++;

for ( i = 0; i < java_array.length; i++ ) {
  CompareValues( java_array[i], test_array[i] );
}

test();

function CompareValues( javaval, testval ) {
  
  new TestCase( SECTION,
		"typeof (" + testval.description +" )",
		testval.type,
		javaval.type );

  
  new TestCase( SECTION,
		"(" + testval.description +" ).getJSClass()",
		testval.jsclass,
		javaval.jsclass );
}
function JavaValue( value ) {
  this.type   = typeof value;
  
  value.getJSClass = Object.prototype.toString;
  this.jsclass = value.getJSClass();
  return this;
}
function TestValue( description  ) {
  this.description = description;
  this.type =  E_TYPE;
  this.jsclass = E_JSCLASS;
  return this;
}
