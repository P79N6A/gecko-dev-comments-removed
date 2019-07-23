





































gTestfile = '15.8.2.1.js';













var SECTION = "15.8.2.1";
var VERSION = "ECMA_1";
var TITLE   = "Math.abs()";
var BUGNUMBER = "77391";
startTest();

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,  
	      "Math.abs.length",
	      1,
              Math.abs.length );

new TestCase( SECTION,
	      "Math.abs()", 
	      Number.NaN,
	      Math.abs() );

new TestCase( SECTION,
	      "Math.abs( void 0 )", 
	      Number.NaN,
	      Math.abs(void 0) );

new TestCase( SECTION,
	      "Math.abs( null )",  
	      0,   
	      Math.abs(null) );

new TestCase( SECTION,
	      "Math.abs( true )", 
	      1,    
	      Math.abs(true) );

new TestCase( SECTION,
	      "Math.abs( false )", 
	      0,     
	      Math.abs(false) );

new TestCase( SECTION,
	      "Math.abs( string primitive)",
	      Number.NaN, 
	      Math.abs("a string primitive") );

new TestCase( SECTION,  
	      "Math.abs( string object )",  
	      Number.NaN,    
	      Math.abs(new String( 'a String object' ))  );

new TestCase( SECTION,  
	      "Math.abs( Number.NaN )", 
	      Number.NaN,
	      Math.abs(Number.NaN) );

new TestCase( SECTION,
	      "Math.abs(0)", 
	      0,
              Math.abs( 0 ) );

new TestCase( SECTION, 
	      "Math.abs( -0 )", 
	      0,  
	      Math.abs(-0) );

new TestCase( SECTION,  
	      "Infinity/Math.abs(-0)",
	      Infinity, 
	      Infinity/Math.abs(-0) );

new TestCase( SECTION,  
	      "Math.abs( -Infinity )",      
	      Number.POSITIVE_INFINITY,  
	      Math.abs( Number.NEGATIVE_INFINITY ) );

new TestCase( SECTION,  
	      "Math.abs( Infinity )",  
	      Number.POSITIVE_INFINITY,
	      Math.abs( Number.POSITIVE_INFINITY ) );

new TestCase( SECTION,  
	      "Math.abs( - MAX_VALUE )",   
	      Number.MAX_VALUE,
	      Math.abs( - Number.MAX_VALUE )       );

new TestCase( SECTION,  
	      "Math.abs( - MIN_VALUE )",
	      Number.MIN_VALUE,
	      Math.abs( -Number.MIN_VALUE )        );

new TestCase( SECTION,  
	      "Math.abs( MAX_VALUE )",  
	      Number.MAX_VALUE,  
	      Math.abs( Number.MAX_VALUE )       );

new TestCase( SECTION, 
	      "Math.abs( MIN_VALUE )",
	      Number.MIN_VALUE, 
	      Math.abs( Number.MIN_VALUE )        );

new TestCase( SECTION,  
	      "Math.abs( -1 )",    
	      1,   
	      Math.abs( -1 )                       );

new TestCase( SECTION,  
	      "Math.abs( new Number( -1 ) )",
	      1,   
	      Math.abs( new Number(-1) )           );

new TestCase( SECTION,  
	      "Math.abs( 1 )",  
	      1, 
	      Math.abs( 1 ) );

new TestCase( SECTION,  
	      "Math.abs( Math.PI )", 
	      Math.PI,   
	      Math.abs( Math.PI ) );

new TestCase( SECTION,
	      "Math.abs( -Math.PI )", 
	      Math.PI,  
	      Math.abs( -Math.PI ) );

new TestCase( SECTION,
	      "Math.abs(-1/100000000)",
	      1/100000000,  
	      Math.abs(-1/100000000) );

new TestCase( SECTION,
	      "Math.abs(-Math.pow(2,32))", 
	      Math.pow(2,32),    
	      Math.abs(-Math.pow(2,32)) );

new TestCase( SECTION,  
	      "Math.abs(Math.pow(2,32))",
	      Math.pow(2,32), 
	      Math.abs(Math.pow(2,32)) );

new TestCase( SECTION,
	      "Math.abs( -0xfff )", 
	      4095,    
	      Math.abs( -0xfff ) );

new TestCase( SECTION,
	      "Math.abs( -0777 )", 
	      511,   
	      Math.abs(-0777 ) );

new TestCase( SECTION,
	      "Math.abs('-1e-1')",  
	      0.1,  
	      Math.abs('-1e-1') );

new TestCase( SECTION, 
	      "Math.abs('0xff')",  
	      255,  
	      Math.abs('0xff') );

new TestCase( SECTION,
	      "Math.abs('077')",   
	      77,   
	      Math.abs('077') );

new TestCase( SECTION, 
	      "Math.abs( 'Infinity' )",
	      Infinity,
	      Math.abs('Infinity') );

new TestCase( SECTION,
	      "Math.abs( '-Infinity' )",
	      Infinity,
	      Math.abs('-Infinity') );

test();
