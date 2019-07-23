





































gTestfile = '15.4.4.4-2.js';




















































var SECTION = "15.4.4.4-1";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Array.prototype.reverse()");

var ARR_PROTOTYPE = Array.prototype;

new TestCase( SECTION, "Array.prototype.reverse.length",           0,      Array.prototype.reverse.length );
new TestCase( SECTION, "delete Array.prototype.reverse.length",    false,  delete Array.prototype.reverse.length );
new TestCase( SECTION, "delete Array.prototype.reverse.length; Array.prototype.reverse.length",    0, eval("delete Array.prototype.reverse.length; Array.prototype.reverse.length") );


new TestCase(   SECTION,
		"var A = new Array();   A.reverse(); A.length",
		0,
		eval("var A = new Array();   A.reverse(); A.length") );

test();

function CheckItems( R, A ) {
  for ( var i = 0; i < R.length; i++ ) {
    new TestCase(
      SECTION,
      "A["+i+ "]",
      R[i],
      A[i] );
  }
}
test();

function Object_1( value ) {
  this.array = value.split(",");
  this.length = this.array.length;
  for ( var i = 0; i < this.length; i++ ) {
    this[i] = eval(this.array[i]);
  }
  this.join = Array.prototype.reverse;
  this.getClass = Object.prototype.toString;
}
function Reverse( array ) {
  var r2 = array.length;
  var k = 0;
  var r3 = Math.floor( r2/2 );
  if ( r3 == k ) {
    return array;
  }

  for ( k = 0;  k < r3; k++ ) {
    var r6 = r2 - k - 1;

    var r7 = k;
    var r8 = String( r6 );

    var r9 = array[r7];
    var r10 = array[r8];

    array[r7] = r10;
    array[r8] = r9;
  }

  return array;
}
function Iterate( array ) {
  for ( var i = 0; i < array.length; i++ ) {

  }
}

function Object_1( value ) {
  this.array = value.split(",");
  this.length = this.array.length;
  for ( var i = 0; i < this.length; i++ ) {
    this[i] = this.array[i];
  }
  this.reverse = Array.prototype.reverse;
  this.getClass = Object.prototype.toString;
}
