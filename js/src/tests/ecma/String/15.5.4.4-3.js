





































gTestfile = '15.5.4.4-3.js';





























var SECTION = "15.5.4.4-3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.prototype.charAt";

writeHeaderToLog( SECTION + " "+ TITLE);

var foo = new MyObject('hello');


new TestCase( SECTION, "var foo = new MyObject('hello'); ", "h", foo.charAt(0)  );
new TestCase( SECTION, "var foo = new MyObject('hello'); ", "e", foo.charAt(1)  );
new TestCase( SECTION, "var foo = new MyObject('hello'); ", "l", foo.charAt(2)  );
new TestCase( SECTION, "var foo = new MyObject('hello'); ", "l", foo.charAt(3)  );
new TestCase( SECTION, "var foo = new MyObject('hello'); ", "o", foo.charAt(4)  );
new TestCase( SECTION, "var foo = new MyObject('hello'); ", "",  foo.charAt(-1)  );
new TestCase( SECTION, "var foo = new MyObject('hello'); ", "",  foo.charAt(5)  );

var boo = new MyObject(true);

new TestCase( SECTION, "var boo = new MyObject(true); ", "t", boo.charAt(0)  );
new TestCase( SECTION, "var boo = new MyObject(true); ", "r", boo.charAt(1)  );
new TestCase( SECTION, "var boo = new MyObject(true); ", "u", boo.charAt(2)  );
new TestCase( SECTION, "var boo = new MyObject(true); ", "e", boo.charAt(3)  );

var noo = new MyObject( Math.PI );

new TestCase( SECTION, "var noo = new MyObject(Math.PI); ", "3", noo.charAt(0)  );
new TestCase( SECTION, "var noo = new MyObject(Math.PI); ", ".", noo.charAt(1)  );
new TestCase( SECTION, "var noo = new MyObject(Math.PI); ", "1", noo.charAt(2)  );
new TestCase( SECTION, "var noo = new MyObject(Math.PI); ", "4", noo.charAt(3)  );
new TestCase( SECTION, "var noo = new MyObject(Math.PI); ", "1", noo.charAt(4)  );
new TestCase( SECTION, "var noo = new MyObject(Math.PI); ", "5", noo.charAt(5)  );
new TestCase( SECTION, "var noo = new MyObject(Math.PI); ", "9", noo.charAt(6)  );

test();

function MyObject (v) {
  this.value      = v;
  this.toString   = new Function( "return this.value +'';" );
  this.valueOf    = new Function( "return this.value" );
  this.charAt     = String.prototype.charAt;
}

