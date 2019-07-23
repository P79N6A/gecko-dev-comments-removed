





































gTestfile = '15.4.1.1.js';

















var SECTION = "15.4.1.1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Array Constructor Called as a Function";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "typeof Array(1,2)",       
	      "object",          
	      typeof Array(1,2) );

new TestCase( SECTION,
	      "(Array(1,2)).toString",   
	      Array.prototype.toString,   
	      (Array(1,2)).toString );

new TestCase( SECTION,
	      "var arr = Array(1,2,3); arr.toString = Object.prototype.toString; arr.toString()",
	      "[object Array]",
	      eval("var arr = Array(1,2,3); arr.toString = Object.prototype.toString; arr.toString()") );

new TestCase( SECTION,
	      "(Array(1,2)).length",     
	      2,                 
	      (Array(1,2)).length );

new TestCase( SECTION,
	      "var arr = (Array(1,2)); arr[0]", 
	      1,          
	      eval("var arr = (Array(1,2)); arr[0]") );

new TestCase( SECTION,
	      "var arr = (Array(1,2)); arr[1]", 
	      2,          
	      eval("var arr = (Array(1,2)); arr[1]") );

new TestCase( SECTION,
	      "var arr = (Array(1,2)); String(arr)", 
	      "1,2", 
	      eval("var arr = (Array(1,2)); String(arr)") );

test();

function ToUint32( n ) {
  n = Number( n );
  if( isNaN(n) || n == 0 || n == Number.POSITIVE_INFINITY ||
      n == Number.NEGATIVE_INFINITY ) {
    return 0;
  }
  var sign = n < 0 ? -1 : 1;

  return ( sign * ( n * Math.floor( Math.abs(n) ) ) ) % Math.pow(2, 32);
}

