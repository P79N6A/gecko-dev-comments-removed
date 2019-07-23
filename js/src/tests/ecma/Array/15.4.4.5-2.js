





































gTestfile = '15.4.4.5-2.js';















































































var SECTION = "15.4.4.5-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Array.prototype.sort(comparefn)";

writeHeaderToLog( SECTION + " "+ TITLE);


var S = new Array();
var item = 0;


S[item++] = "var A = new Array()";


S[item++] = "var A = new Array( true )";


S[item++] = "var A = new Array( true, false, new Boolean(true), new Boolean(false), 'true', 'false' )";

S[item++] = "var A = new Array(); A[3] = 'undefined'; A[6] = null; A[8] = 'null'; A[0] = void 0";

S[item] = "var A = new Array( ";

var limit = 0x0061;
for ( var i = 0x007A; i >= limit; i-- ) {
  S[item] += "\'"+ String.fromCharCode(i) +"\'" ;
  if ( i > limit ) {
    S[item] += ",";
  }
}

S[item] += ")";

for ( var i = 0; i < S.length; i++ ) {
  CheckItems( S[i] );
}

test();

function CheckItems( S ) {
  eval( S );
  var E = Sort( A );

  new TestCase(   SECTION,
		  S +";  A.sort(Compare); A.length",
		  E.length,
		  eval( S + "; A.sort(Compare); A.length") );

  for ( var i = 0; i < E.length; i++ ) {
    new TestCase(
      SECTION,
      "A["+i+ "].toString()",
      E[i] +"",
      A[i] +"");

    if ( A[i] == void 0 && typeof A[i] == "undefined" ) {
      new TestCase(
	SECTION,
	"typeof A["+i+ "]",
	typeof E[i],
	typeof A[i] );
    }
  }
}
function Object_1( value ) {
  this.array = value.split(",");
  this.length = this.array.length;
  for ( var i = 0; i < this.length; i++ ) {
    this[i] = eval(this.array[i]);
  }
  this.sort = Array.prototype.sort;
  this.getClass = Object.prototype.toString;
}
function Sort( a ) {
  var r1 = a.length;
  for ( i = 0; i < a.length; i++ ) {
    for ( j = i+1; j < a.length; j++ ) {
      var lo = a[i];
      var hi = a[j];
      var c = Compare( lo, hi );
      if ( c == 1 ) {
	a[i] = hi;
	a[j] = lo;
      }
    }
  }
  return a;
}
function Compare( x, y ) {
  if ( x == void 0 && y == void 0  && typeof x == "undefined" && typeof y == "undefined" ) {
    return +0;
  }
  if ( x == void 0  && typeof x == "undefined" ) {
    return 1;
  }
  if ( y == void 0 && typeof y == "undefined" ) {
    return -1;
  }
  x = String(x);
  y = String(y);
  if ( x < y ) {
    return 1;
  }
  if ( x > y ) {
    return -1;
  }
  return 0;
}
