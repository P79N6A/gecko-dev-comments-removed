





















var SECTION = "15.8.1"
  var VERSION = "ECMA_1";
startTest();
var TITLE   = "Value Properties of the Math Object";

writeHeaderToLog( SECTION + " "+ TITLE);


new TestCase( "15.8.1.1", "Math.E",            
	      2.7182818284590452354, 
	      Math.E );

new TestCase( "15.8.1.1",
	      "typeof Math.E",     
	      "number",              
	      typeof Math.E );

new TestCase( "15.8.1.2",
	      "Math.LN10",         
	      2.302585092994046,     
	      Math.LN10 );

new TestCase( "15.8.1.2",
	      "typeof Math.LN10",  
	      "number",              
	      typeof Math.LN10 );

new TestCase( "15.8.1.3",
	      "Math.LN2",         
	      0.6931471805599453,    
	      Math.LN2 );

new TestCase( "15.8.1.3",
	      "typeof Math.LN2",   
	      "number",              
	      typeof Math.LN2 );

new TestCase( "15.8.1.4",
	      "Math.LOG2E",        
	      1.4426950408889634,    
	      Math.LOG2E );

new TestCase( "15.8.1.4",
	      "typeof Math.LOG2E", 
	      "number",              
	      typeof Math.LOG2E );

new TestCase( "15.8.1.5",
	      "Math.LOG10E",       
	      0.4342944819032518,    
	      Math.LOG10E);

new TestCase( "15.8.1.5",
	      "typeof Math.LOG10E",
	      "number",              
	      typeof Math.LOG10E);

new TestCase( "15.8.1.6",
	      "Math.PI",           
	      3.14159265358979323846,
	      Math.PI );

new TestCase( "15.8.1.6",
	      "typeof Math.PI",    
	      "number",              
	      typeof Math.PI );

new TestCase( "15.8.1.7",
	      "Math.SQRT1_2",      
	      0.7071067811865476,    
	      Math.SQRT1_2);

new TestCase( "15.8.1.7",
	      "typeof Math.SQRT1_2",
	      "number",             
	      typeof Math.SQRT1_2);

new TestCase( "15.8.1.8",
	      "Math.SQRT2",        
	      1.4142135623730951,    
	      Math.SQRT2 );

new TestCase( "15.8.1.8",
	      "typeof Math.SQRT2", 
	      "number",              
	      typeof Math.SQRT2 );

new TestCase( SECTION, 
	      "var MATHPROPS='';for( p in Math ){ MATHPROPS +=p; };MATHPROPS",
	      "",
	      eval("var MATHPROPS='';for( p in Math ){ MATHPROPS +=p; };MATHPROPS") );

test();
