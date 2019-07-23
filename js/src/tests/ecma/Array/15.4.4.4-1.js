





































gTestfile = '15.4.4.4-1.js';



















































var SECTION = "15.4.4.4-1";
var VERSION = "ECMA_1";
var BUGNUMBER="123724";
startTest();

writeHeaderToLog( SECTION + " Array.prototype.reverse()");

var ARR_PROTOTYPE = Array.prototype;

new TestCase( SECTION,
	      "Array.prototype.reverse.length",          
	      0,     
	      Array.prototype.reverse.length );

new TestCase( SECTION,
	      "delete Array.prototype.reverse.length",   
	      false, 
	      delete Array.prototype.reverse.length );

new TestCase( SECTION,
	      "delete Array.prototype.reverse.length; Array.prototype.reverse.length",   
	      0,
	      eval("delete Array.prototype.reverse.length; Array.prototype.reverse.length") );


new TestCase( SECTION,
	      "var A = new Array();   A.reverse(); A.length",
	      0,
	      eval("var A = new Array();   A.reverse(); A.length") );


var A = new Array(true);
var R = Reverse(A);

new TestCase( SECTION,
	      "var A = new Array(true);   A.reverse(); A.length",
	      R.length,
	      eval("var A = new Array(true);   A.reverse(); A.length") );

CheckItems( R, A );


var S = "var A = new Array( true,false )";
eval(S);
var R = Reverse(A);

new TestCase( SECTION,
	      S +";  A.reverse(); A.length",
	      R.length,
	      eval( S + "; A.reverse(); A.length") );

CheckItems(  R, A );


var S = "var A = new Array( true,false,null )";
eval(S);
var R = Reverse(A);

new TestCase( SECTION,
	      S +";  A.reverse(); A.length",
	      R.length,
	      eval( S + "; A.reverse(); A.length") );

CheckItems( R, A );


var S = "var A = new Array( true,false,null,void 0 )";
eval(S);
var R = Reverse(A);

new TestCase( SECTION,
	      S +";  A.reverse(); A.length",
	      R.length,
	      eval( S + "; A.reverse(); A.length") );
CheckItems( R, A );



var S = "var A = new Array(); A[8] = 'hi', A[3] = 'yo'";
eval(S);
var R = Reverse(A);

new TestCase( SECTION,
	      S +";  A.reverse(); A.length",
	      R.length,
	      eval( S + "; A.reverse(); A.length") );

CheckItems( R, A );


var OBJECT_OBJECT = new Object();
var FUNCTION_OBJECT = new Function( 'return this' );
var BOOLEAN_OBJECT = new Boolean;
var DATE_OBJECT = new Date(0);
var STRING_OBJECT = new String('howdy');
var NUMBER_OBJECT = new Number(Math.PI);
var ARRAY_OBJECT= new Array(1000);

var args = "null, void 0, Math.pow(2,32), 1.234e-32, OBJECT_OBJECT, BOOLEAN_OBJECT, FUNCTION_OBJECT, DATE_OBJECT, STRING_OBJECT,"+
  "ARRAY_OBJECT, NUMBER_OBJECT, Math, true, false, 123, '90210'";

var S = "var A = new Array("+args+")";
eval(S);
var R = Reverse(A);

new TestCase( SECTION,
	      S +";  A.reverse(); A.length",
	      R.length,
	      eval( S + "; A.reverse(); A.length") );

CheckItems( R, A );

var limit = 1000;
var args = "";
for (var i = 0; i < limit; i++ ) {
  args += i +"";
  if ( i + 1 < limit ) {
    args += ",";
  }
}

var S = "var A = new Array("+args+")";
eval(S);
var R = Reverse(A);

new TestCase( SECTION,
	      S +";  A.reverse(); A.length",
	      R.length,
	      eval( S + "; A.reverse(); A.length") );

CheckItems( R, A );

var S = "var MYOBJECT = new Object_1( \"void 0, 1, null, 2, \'\'\" )";
eval(S);
var R = Reverse( A );

new TestCase( SECTION,
	      S +";  A.reverse(); A.length",
	      R.length,
	      eval( S + "; A.reverse(); A.length") );

CheckItems( R, A );

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
