





































gTestfile = '15.4.1.2.js';

















var SECTION = "15.4.1.2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Array Constructor Called as a Function:  Array(len)";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION, 
	      "(Array()).length",            
	      0,                             
	      (Array()).length );

new TestCase( SECTION,
	      "(Array(0)).length",           
	      0,                             
	      (Array(0)).length );

new TestCase( SECTION,
	      "(Array(1)).length",           
	      1,                             
	      (Array(1)).length );

new TestCase( SECTION,
	      "(Array(10)).length",          
	      10,                            
	      (Array(10)).length );

new TestCase( SECTION,
	      "(Array('1')).length",         
	      1,                             
	      (Array('1')).length );

new TestCase( SECTION,
	      "(Array(1000)).length",        
	      1000,                          
	      (Array(1000)).length );

new TestCase( SECTION,
	      "(Array('1000')).length",      
	      1,                             
	      (Array('1000')).length );

new TestCase( SECTION,
	      "(Array(4294967295)).length",  
	      ToUint32(4294967295),          
	      (Array(4294967295)).length );

new TestCase( SECTION,
	      "(Array(Math.pow(2,31)-1)).length",    
	      ToUint32(Math.pow(2,31)-1),    
	      (Array(Math.pow(2,31)-1)).length );

new TestCase( SECTION,
	      "(Array(Math.pow(2,31))).length",      
	      ToUint32(Math.pow(2,31)),      
	      (Array(Math.pow(2,31))).length );

new TestCase( SECTION,
	      "(Array(Math.pow(2,31)+1)).length",    
	      ToUint32(Math.pow(2,31)+1),    
	      (Array(Math.pow(2,31)+1)).length );

new TestCase( SECTION,
	      "(Array('8589934592')).length",
	      1,                             
	      (Array("8589934592")).length );

new TestCase( SECTION,
	      "(Array('4294967296')).length",
	      1,                             
	      (Array("4294967296")).length );

new TestCase( SECTION,
	      "(Array(1073741823)).length",  
	      ToUint32(1073741823),          
	      (Array(1073741823)).length );

new TestCase( SECTION,
	      "(Array(1073741824)).length",  
	      ToUint32(1073741824),	       
	      (Array(1073741824)).length );

new TestCase( SECTION,
	      "(Array('a string')).length",  
	      1,                             
	      (Array("a string")).length );

test();

function ToUint32( n ) {
  n = Number( n );
  var sign = ( n < 0 ) ? -1 : 1;

  if ( Math.abs( n ) == 0 || Math.abs( n ) == Number.POSITIVE_INFINITY) {
    return 0;
  }
  n = sign * Math.floor( Math.abs(n) )

    n = n % Math.pow(2,32);

  if ( n < 0 ){
    n += Math.pow(2,32);
  }

  return ( n );
}
