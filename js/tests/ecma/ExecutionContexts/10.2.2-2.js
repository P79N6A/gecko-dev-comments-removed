





































gTestfile = '10.2.2-2.js';


























var SECTION = "10.2.2-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Eval Code";

writeHeaderToLog( SECTION + " "+ TITLE);



var OBJECT = new MyObject( "hello" );
var GLOBAL_PROPERTIES = new Array();
var i = 0;

for ( p in this ) {
  GLOBAL_PROPERTIES[i++] = p;
}

with ( OBJECT ) {
  var THIS = this;
  new TestCase( SECTION,
		"eval( 'this == THIS' )",                 
		true,              
		eval("this == THIS") );
  new TestCase( SECTION,
		"this in a with() block",                 
		GLOBAL, 
		this+"" );
  new TestCase( SECTION,
		"new MyObject('hello').value",            
		"hello",           
		value );
  new TestCase( SECTION,
		"eval(new MyObject('hello').value)",      
		"hello",           
		eval("value") );
  new TestCase( SECTION,
		"new MyObject('hello').getClass()",       
		"[object Object]", 
		getClass() );
  new TestCase( SECTION,
		"eval(new MyObject('hello').getClass())", 
		"[object Object]", 
		eval("getClass()") );
  new TestCase( SECTION,
		"eval(new MyObject('hello').toString())", 
		"hello", 
		eval("toString()") );
  new TestCase( SECTION,
		"eval('getClass') == Object.prototype.toString", 
		true, 
		eval("getClass") == Object.prototype.toString );

  for ( i = 0; i < GLOBAL_PROPERTIES.length; i++ ) {
    new TestCase( SECTION, GLOBAL_PROPERTIES[i] +
		  " == THIS["+GLOBAL_PROPERTIES[i]+"]", true,
		  eval(GLOBAL_PROPERTIES[i]) == eval( "THIS[GLOBAL_PROPERTIES[i]]") );
  }

}

test();

function MyObject( value ) {
  this.value = value;
  this.getClass = Object.prototype.toString;
  this.toString = new Function( "return this.value+''" );
  return this;
}
