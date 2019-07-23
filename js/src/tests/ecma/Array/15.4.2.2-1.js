





































gTestfile = '15.4.2.2-1.js';

































var SECTION = "15.4.2.2-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The Array Constructor:  new Array( len )";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "new Array(0)",            
	      "",                
	      (new Array(0)).toString() );

new TestCase( SECTION,
	      "typeof new Array(0)",     
	      "object",          
	      (typeof new Array(0)) );

new TestCase( SECTION,
	      "(new Array(0)).length",   
	      0,                 
	      (new Array(0)).length );

new TestCase( SECTION,
	      "(new Array(0)).toString",
	      Array.prototype.toString,   
	      (new Array(0)).toString );

new TestCase( SECTION,  
	      "new Array(1)",           
	      "",                
	      (new Array(1)).toString() );

new TestCase( SECTION,  
	      "new Array(1).length",    
	      1,                 
	      (new Array(1)).length );

new TestCase( SECTION,  
	      "(new Array(1)).toString",
	      Array.prototype.toString,  
	      (new Array(1)).toString );

new TestCase( SECTION,
	      "(new Array(-0)).length",                      
	      0, 
	      (new Array(-0)).length );

new TestCase( SECTION,
	      "(new Array(0)).length",                       
	      0, 
	      (new Array(0)).length );

new TestCase( SECTION,
	      "(new Array(10)).length",          
	      10,        
	      (new Array(10)).length );

new TestCase( SECTION,
	      "(new Array('1')).length",         
	      1,         
	      (new Array('1')).length );

new TestCase( SECTION,
	      "(new Array(1000)).length",        
	      1000,      
	      (new Array(1000)).length );

new TestCase( SECTION,
	      "(new Array('1000')).length",      
	      1,         
	      (new Array('1000')).length );

new TestCase( SECTION,
	      "(new Array(4294967295)).length",  
	      ToUint32(4294967295),  
	      (new Array(4294967295)).length );

new TestCase( SECTION,
	      "(new Array('8589934592')).length",
	      1,                     
	      (new Array("8589934592")).length );

new TestCase( SECTION,
	      "(new Array('4294967296')).length",
	      1,                     
	      (new Array("4294967296")).length );

new TestCase( SECTION,
	      "(new Array(1073741824)).length",  
	      ToUint32(1073741824),
	      (new Array(1073741824)).length );

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
