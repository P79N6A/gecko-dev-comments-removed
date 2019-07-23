





































gTestfile = '15.4.4.3-1.js';


















var SECTION = "15.4.4.3-1";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Array.prototype.join()");

var ARR_PROTOTYPE = Array.prototype;

new TestCase( SECTION, "Array.prototype.join.length",           1,      Array.prototype.join.length );
new TestCase( SECTION, "delete Array.prototype.join.length",    false,  delete Array.prototype.join.length );
new TestCase( SECTION, "delete Array.prototype.join.length; Array.prototype.join.length",    1, eval("delete Array.prototype.join.length; Array.prototype.join.length") );



new TestCase(   SECTION,
		"var TEST_ARRAY = new Array(); TEST_ARRAY.join()",
		"",
		eval("var TEST_ARRAY = new Array(); TEST_ARRAY.join()") );



new TestCase(   SECTION,
		"var TEST_ARRAY = new Array(); TEST_ARRAY.join(' ')",
		"",
		eval("var TEST_ARRAY = new Array(); TEST_ARRAY.join(' ')") );


new TestCase(   SECTION,
		"var TEST_ARRAY = new Array(null, void 0, true, false, 123, new Object(), new Boolean(true) ); TEST_ARRAY.join('&')",
		"&&true&false&123&[object Object]&true",
		eval("var TEST_ARRAY = new Array(null, void 0, true, false, 123, new Object(), new Boolean(true) ); TEST_ARRAY.join('&')") );


new TestCase(   SECTION,
		"var TEST_ARRAY = new Array(null, void 0, true, false, 123, new Object(), new Boolean(true) ); TEST_ARRAY.join('')",
		"truefalse123[object Object]true",
		eval("var TEST_ARRAY = new Array(null, void 0, true, false, 123, new Object(), new Boolean(true) ); TEST_ARRAY.join('')") );


new TestCase(   SECTION,
		"var TEST_ARRAY = new Array(null, void 0, true, false, 123, new Object(), new Boolean(true) ); TEST_ARRAY.join(void 0)",
		",,true,false,123,[object Object],true",
		eval("var TEST_ARRAY = new Array(null, void 0, true, false, 123, new Object(), new Boolean(true) ); TEST_ARRAY.join(void 0)") );


new TestCase(   SECTION,
		"var TEST_ARRAY = new Array(null, void 0, true, false, 123, new Object(), new Boolean(true) ); TEST_ARRAY.join()",
		",,true,false,123,[object Object],true",
		eval("var TEST_ARRAY = new Array(null, void 0, true, false, 123, new Object(), new Boolean(true) ); TEST_ARRAY.join()") );


new TestCase(   SECTION,
		"var TEST_ARRAY = new Array(null, void 0, true, false, 123, new Object(), new Boolean(true) ); TEST_ARRAY.join('\v')",
		decodeURIComponent("%0B%0Btrue%0Bfalse%0B123%0B[object Object]%0Btrue"),
		eval("var TEST_ARRAY = new Array(null, void 0, true, false, 123, new Object(), new Boolean(true) ); TEST_ARRAY.join('\v')") );


new TestCase(   SECTION,
		"var TEST_ARRAY = new Array(true) ); TEST_ARRAY.join('\v')",
		"true",
		eval("var TEST_ARRAY = new Array(true); TEST_ARRAY.join('\v')") );


SEPARATOR = "\t"
  TEST_LENGTH = 100;
TEST_STRING = "";
ARGUMENTS = "";
TEST_RESULT = "";

for ( var index = 0; index < TEST_LENGTH; index++ ) {
  ARGUMENTS   += index;
  ARGUMENTS   += ( index == TEST_LENGTH -1 ) ? "" : ",";

  TEST_RESULT += index;
  TEST_RESULT += ( index == TEST_LENGTH -1 ) ? "" : SEPARATOR;
}

TEST_ARRAY = eval( "new Array( "+ARGUMENTS +")" );

new TestCase( SECTION,
	      "TEST_ARRAY.join("+SEPARATOR+")",  
	      TEST_RESULT,   
	      TEST_ARRAY.join( SEPARATOR ) );

new TestCase( SECTION,
	      "(new Array( Boolean(true), Boolean(false), null,  void 0, Number(1e+21), Number(1e-7))).join()",
	      "true,false,,,1e+21,1e-7",
	      (new Array( Boolean(true), Boolean(false), null,  void 0, Number(1e+21), Number(1e-7))).join() );


new TestCase(   SECTION,
		"var OB = new Object_1('true,false,111,0.5,1.23e6,NaN,void 0,null'); OB.join(':')",
		"true:false:111:0.5:1230000:NaN::",
		eval("var OB = new Object_1('true,false,111,0.5,1.23e6,NaN,void 0,null'); OB.join(':')") );

test();

function Object_1( value ) {
  this.array = value.split(",");
  this.length = this.array.length;
  for ( var i = 0; i < this.length; i++ ) {
    this[i] = eval(this.array[i]);
  }
  this.join = Array.prototype.join;
  this.getClass = Object.prototype.toString;
}
