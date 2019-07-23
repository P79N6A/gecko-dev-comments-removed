





































gTestfile = '15.5.4.6-2.js';














































var SECTION = "15.5.4.6-2";
var VERSION = "ECMA_1";
var TITLE   = "String.protoype.indexOf";
var BUGNUMBER="105721";

startTest();

writeHeaderToLog( SECTION + " "+ TITLE);






new TestCase( SECTION,
              "function f() { return this; }; function g() { var h = f; return h(); }; g().toString()",   
              GLOBAL, 
              g().toString()
  );


new TestCase( SECTION, "String.prototype.indexOf.length",                                               1,     String.prototype.indexOf.length );
new TestCase( SECTION, "String.prototype.indexOf.length = null; String.prototype.indexOf.length",       1,     eval("String.prototype.indexOf.length = null; String.prototype.indexOf.length") );
new TestCase( SECTION, "delete String.prototype.indexOf.length",                                        false,  delete String.prototype.indexOf.length );
new TestCase( SECTION, "delete String.prototype.indexOf.length; String.prototype.indexOf.length",       1,      eval("delete String.prototype.indexOf.length; String.prototype.indexOf.length") );

new TestCase( SECTION,
              "var s = new String(); s.indexOf()",    
              -1,    
              eval("var s = new String(); s.indexOf()") );





var TEST_STRING = "";

for ( var u = 0x00A1; u <= 0x00FF; u++ ) {
  TEST_STRING += String.fromCharCode( u );
}

for ( var u = 0x00A1, i = 0; u <= 0x00FF; u++, i++ ) {
  new TestCase(   SECTION,
		  "TEST_STRING.indexOf( " + String.fromCharCode(u) + " )",
		  i,
		  TEST_STRING.indexOf( String.fromCharCode(u) ) );
}
for ( var u = 0x00A1, i = 0; u <= 0x00FF; u++, i++ ) {
  new TestCase(   SECTION,
		  "TEST_STRING.indexOf( " + String.fromCharCode(u) + ", void 0 )",
		  i,
		  TEST_STRING.indexOf( String.fromCharCode(u), void 0 ) );
}



var foo = new MyObject('hello');

new TestCase( SECTION, "var foo = new MyObject('hello');foo.indexOf('h')", 0, foo.indexOf("h")  );
new TestCase( SECTION, "var foo = new MyObject('hello');foo.indexOf('e')", 1, foo.indexOf("e")  );
new TestCase( SECTION, "var foo = new MyObject('hello');foo.indexOf('l')", 2, foo.indexOf("l")  );
new TestCase( SECTION, "var foo = new MyObject('hello');foo.indexOf('l')", 2, foo.indexOf("l")  );
new TestCase( SECTION, "var foo = new MyObject('hello');foo.indexOf('o')", 4, foo.indexOf("o")  );
new TestCase( SECTION, "var foo = new MyObject('hello');foo.indexOf('X')", -1,  foo.indexOf("X")  );
new TestCase( SECTION, "var foo = new MyObject('hello');foo.indexOf(5) ", -1,  foo.indexOf(5)  );

var boo = new MyObject(true);

new TestCase( SECTION, "var boo = new MyObject(true);boo.indexOf('t')", 0, boo.indexOf("t")  );
new TestCase( SECTION, "var boo = new MyObject(true);boo.indexOf('r')", 1, boo.indexOf("r")  );
new TestCase( SECTION, "var boo = new MyObject(true);boo.indexOf('u')", 2, boo.indexOf("u")  );
new TestCase( SECTION, "var boo = new MyObject(true);boo.indexOf('e')", 3, boo.indexOf("e")  );
new TestCase( SECTION, "var boo = new MyObject(true);boo.indexOf('true')", 0, boo.indexOf("true")  );
new TestCase( SECTION, "var boo = new MyObject(true);boo.indexOf('rue')", 1, boo.indexOf("rue")  );
new TestCase( SECTION, "var boo = new MyObject(true);boo.indexOf('ue')", 2, boo.indexOf("ue")  );
new TestCase( SECTION, "var boo = new MyObject(true);boo.indexOf('oy')", -1, boo.indexOf("oy")  );


var noo = new MyObject( Math.PI );
new TestCase( SECTION, "var noo = new MyObject(Math.PI); noo.indexOf('3') ", 0, noo.indexOf('3')  );
new TestCase( SECTION, "var noo = new MyObject(Math.PI); noo.indexOf('.') ", 1, noo.indexOf('.')  );
new TestCase( SECTION, "var noo = new MyObject(Math.PI); noo.indexOf('1') ", 2, noo.indexOf('1')  );
new TestCase( SECTION, "var noo = new MyObject(Math.PI); noo.indexOf('4') ", 3, noo.indexOf('4')  );
new TestCase( SECTION, "var noo = new MyObject(Math.PI); noo.indexOf('1') ", 2, noo.indexOf('1')  );
new TestCase( SECTION, "var noo = new MyObject(Math.PI); noo.indexOf('5') ", 5, noo.indexOf('5')  );
new TestCase( SECTION, "var noo = new MyObject(Math.PI); noo.indexOf('9') ", 6, noo.indexOf('9')  );

new TestCase( SECTION,
	      "var arr = new Array('new','zoo','revue'); arr.indexOf = String.prototype.indexOf; arr.indexOf('new')",
	      0,
	      eval("var arr = new Array('new','zoo','revue'); arr.indexOf = String.prototype.indexOf; arr.indexOf('new')") );

new TestCase( SECTION,
	      "var arr = new Array('new','zoo','revue'); arr.indexOf = String.prototype.indexOf; arr.indexOf(',zoo,')",
	      3,
	      eval("var arr = new Array('new','zoo','revue'); arr.indexOf = String.prototype.indexOf; arr.indexOf(',zoo,')") );

new TestCase( SECTION,
	      "var obj = new Object(); obj.indexOf = String.prototype.indexOf; obj.indexOf('[object Object]')",
	      0,
	      eval("var obj = new Object(); obj.indexOf = String.prototype.indexOf; obj.indexOf('[object Object]')") );

new TestCase( SECTION,
	      "var obj = new Object(); obj.indexOf = String.prototype.indexOf; obj.indexOf('bject')",
	      2,
	      eval("var obj = new Object(); obj.indexOf = String.prototype.indexOf; obj.indexOf('bject')") );

new TestCase( SECTION,
	      "var f = new Object( String.prototype.indexOf ); f('"+GLOBAL+"')",
	      0,
	      eval("var f = new Object( String.prototype.indexOf ); f('"+GLOBAL+"')") );

new TestCase( SECTION,
	      "var f = new Function(); f.toString = Object.prototype.toString; f.indexOf = String.prototype.indexOf; f.indexOf('[object Function]')",
	      0,
	      eval("var f = new Function(); f.toString = Object.prototype.toString; f.indexOf = String.prototype.indexOf; f.indexOf('[object Function]')") );

new TestCase( SECTION,
	      "var b = new Boolean(); b.indexOf = String.prototype.indexOf; b.indexOf('true')",
	      -1,
	      eval("var b = new Boolean(); b.indexOf = String.prototype.indexOf; b.indexOf('true')") );

new TestCase( SECTION,
	      "var b = new Boolean(); b.indexOf = String.prototype.indexOf; b.indexOf('false', 1)",
	      -1,
	      eval("var b = new Boolean(); b.indexOf = String.prototype.indexOf; b.indexOf('false', 1)") );

new TestCase( SECTION,
	      "var b = new Boolean(); b.indexOf = String.prototype.indexOf; b.indexOf('false', 0)",
	      0,
	      eval("var b = new Boolean(); b.indexOf = String.prototype.indexOf; b.indexOf('false', 0)") );

new TestCase( SECTION,
	      "var n = new Number(1e21); n.indexOf = String.prototype.indexOf; n.indexOf('e')",
	      1,
	      eval("var n = new Number(1e21); n.indexOf = String.prototype.indexOf; n.indexOf('e')") );

new TestCase( SECTION,
	      "var n = new Number(-Infinity); n.indexOf = String.prototype.indexOf; n.indexOf('-')",
	      0,
	      eval("var n = new Number(-Infinity); n.indexOf = String.prototype.indexOf; n.indexOf('-')") );

new TestCase( SECTION,
	      "var n = new Number(0xFF); n.indexOf = String.prototype.indexOf; n.indexOf('5')",
	      1,
	      eval("var n = new Number(0xFF); n.indexOf = String.prototype.indexOf; n.indexOf('5')") );

new TestCase( SECTION,
	      "var m = Math; m.indexOf = String.prototype.indexOf; m.indexOf( 'Math' )",
	      8,
	      eval("var m = Math; m.indexOf = String.prototype.indexOf; m.indexOf( 'Math' )") );


new TestCase( SECTION,
	      "var d = new Date(0); d.indexOf = String.prototype.indexOf; d.getTimezoneOffset()>0 ? d.indexOf('31') : d.indexOf('01')",
	      8,
	      eval("var d = new Date(0); d.indexOf = String.prototype.indexOf; d.getTimezoneOffset()>0 ? d.indexOf('31') : d.indexOf('01')") );

test();

function f() {
  return this;
}
function g() {
  var h = f;
  return h();
}

function MyObject (v) {
  this.value      = v;
  this.toString   = new Function ( "return this.value +\"\"");
  this.indexOf     = String.prototype.indexOf;
}

