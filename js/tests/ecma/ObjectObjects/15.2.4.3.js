





































gTestfile = '15.2.4.3.js';

















var SECTION = "15.2.4.3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Object.prototype.valueOf()";

writeHeaderToLog( SECTION + " "+ TITLE);


var myarray = new Array();
myarray.valueOf = Object.prototype.valueOf;
var myboolean = new Boolean();
myboolean.valueOf = Object.prototype.valueOf;
var myfunction = new Function();
myfunction.valueOf = Object.prototype.valueOf;
var myobject = new Object();
myobject.valueOf = Object.prototype.valueOf;
var mymath = Math;
mymath.valueOf = Object.prototype.valueOf;
var mydate = new Date();
mydate.valueOf = Object.prototype.valueOf;
var mynumber = new Number();
mynumber.valueOf = Object.prototype.valueOf;
var mystring = new String();
mystring.valueOf = Object.prototype.valueOf;

new TestCase( SECTION,  "Object.prototype.valueOf.length",      0,      Object.prototype.valueOf.length );

new TestCase( SECTION,
	      "myarray = new Array(); myarray.valueOf = Object.prototype.valueOf; myarray.valueOf()",
	      myarray,
	      myarray.valueOf() );
new TestCase( SECTION,
	      "myboolean = new Boolean(); myboolean.valueOf = Object.prototype.valueOf; myboolean.valueOf()",
	      myboolean,
	      myboolean.valueOf() );
new TestCase( SECTION,
	      "myfunction = new Function(); myfunction.valueOf = Object.prototype.valueOf; myfunction.valueOf()",
	      myfunction,
	      myfunction.valueOf() );
new TestCase( SECTION,
	      "myobject = new Object(); myobject.valueOf = Object.prototype.valueOf; myobject.valueOf()",
	      myobject,
	      myobject.valueOf() );
new TestCase( SECTION,
	      "mymath = Math; mymath.valueOf = Object.prototype.valueOf; mymath.valueOf()",
	      mymath,
	      mymath.valueOf() );
new TestCase( SECTION,
	      "mynumber = new Number(); mynumber.valueOf = Object.prototype.valueOf; mynumber.valueOf()",
	      mynumber,
	      mynumber.valueOf() );
new TestCase( SECTION,
	      "mystring = new String(); mystring.valueOf = Object.prototype.valueOf; mystring.valueOf()",
	      mystring,
	      mystring.valueOf() );
new TestCase( SECTION,
	      "mydate = new Date(); mydate.valueOf = Object.prototype.valueOf; mydate.valueOf()",
	      mydate,
	      mydate.valueOf() );

test();
