





































gTestfile = '15.3.2.1-1.js';


















var SECTION = "15.3.2.1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The Function Constructor";

writeHeaderToLog( SECTION + " "+ TITLE);

var MyObject = new Function( "value", "this.value = value; this.valueOf = new Function( 'return this.value' ); this.toString = new Function( 'return String(this.value);' )" );

var myfunc = new Function();




myfunc.toString = Object.prototype.toString;

new TestCase( SECTION,  "myfunc = new Function(); myfunc.toString = Object.prototype.toString; myfunc.toString()",
	      "[object Function]",
	      myfunc.toString() );

new TestCase( SECTION, 
	      "myfunc.length",                           
	      0,                     
	      myfunc.length );

new TestCase( SECTION,
	      "myfunc.prototype.toString()",
	      "[object Object]", 
	      myfunc.prototype.toString() );

new TestCase( SECTION,
	      "myfunc.prototype.constructor",   
	      myfunc,   
	      myfunc.prototype.constructor );

new TestCase( SECTION,
	      "myfunc.arguments",  
	      null,  
	      myfunc.arguments );

new TestCase( SECTION,
	      "var OBJ = new MyObject(true); OBJ.valueOf()",
	      true, 
	      eval("var OBJ = new MyObject(true); OBJ.valueOf()") );

new TestCase( SECTION,
	      "OBJ.toString()",  
	      "true", 
	      OBJ.toString() );

new TestCase( SECTION,
	      "OBJ.toString = Object.prototype.toString; OBJ.toString()", "[object Object]",
	      eval("OBJ.toString = Object.prototype.toString; OBJ.toString()") );

new TestCase( SECTION, 
	      "MyObject.toString = Object.prototype.toString; MyObject.toString()",
	      "[object Function]", 
	      eval("MyObject.toString = Object.prototype.toString; MyObject.toString()") );

new TestCase( SECTION,
	      "MyObject.length", 
	      1,
	      MyObject.length );

new TestCase( SECTION,
	      "MyObject.prototype.constructor",
              MyObject,
	      MyObject.prototype.constructor );

new TestCase( SECTION,
	      "MyObject.arguments",  
	      null, 
	      MyObject.arguments );

test();
