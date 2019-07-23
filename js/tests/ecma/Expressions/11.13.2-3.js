





































gTestfile = '11.13.2-3.js';

























var SECTION = "11.13.2-3";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Compound Assignment: +=");



new TestCase( SECTION,   
              "VAR1 = NaN; VAR2=1; VAR1 %= VAR2",      
              Number.NaN,
              eval("VAR1 = Number.NaN; VAR2=1; VAR1 %= VAR2") );

new TestCase( SECTION,   
              "VAR1 = NaN; VAR2=1; VAR1 %= VAR2; VAR1",
              Number.NaN,
              eval("VAR1 = Number.NaN; VAR2=1; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = NaN; VAR2=0; VAR1 %= VAR2",      
              Number.NaN,
              eval("VAR1 = Number.NaN; VAR2=0; VAR1 %= VAR2") );

new TestCase( SECTION,   
              "VAR1 = NaN; VAR2=0; VAR1 %= VAR2; VAR1",
              Number.NaN,
              eval("VAR1 = Number.NaN; VAR2=0; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = 0; VAR2=NaN; VAR1 %= VAR2",      
              Number.NaN,
              eval("VAR1 = 0; VAR2=Number.NaN; VAR1 %= VAR2") );

new TestCase( SECTION,   
              "VAR1 = 0; VAR2=NaN; VAR1 %= VAR2; VAR1",
              Number.NaN,
              eval("VAR1 = 0; VAR2=Number.NaN; VAR1 %= VAR2; VAR1") );



new TestCase( SECTION,   
              "VAR1 = Infinity; VAR2= Infinity; VAR1 %= VAR2; VAR1",  
              Number.NaN,     
              eval("VAR1 = Number.POSITIVE_INFINITY; VAR2 = Number.POSITIVE_INFINITY; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = Infinity; VAR2= -Infinity; VAR1 %= VAR2; VAR1", 
              Number.NaN,     
              eval("VAR1 = Number.POSITIVE_INFINITY; VAR2 = Number.NEGATIVE_INFINITY; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 =-Infinity; VAR2= Infinity; VAR1 %= VAR2; VAR1",  
              Number.NaN,     
              eval("VAR1 = Number.NEGATIVE_INFINITY; VAR2 = Number.POSITIVE_INFINITY; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 =-Infinity; VAR2=-Infinity; VAR1 %= VAR2; VAR1",  
              Number.NaN,     
              eval("VAR1 = Number.NEGATIVE_INFINITY; VAR2 = Number.NEGATIVE_INFINITY; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = 0; VAR2= Infinity; VAR2 %= VAR1",   
              Number.NaN,     
              eval("VAR1 = 0; VAR2 = Number.POSITIVE_INFINITY; VAR2 %= VAR1; VAR2") );

new TestCase( SECTION,   
              "VAR1 = -0; VAR2= Infinity; VAR2 %= VAR1",  
              Number.NaN,     
              eval("VAR1 = -0; VAR2 = Number.POSITIVE_INFINITY; VAR2 %= VAR1; VAR2") );

new TestCase( SECTION,   
              "VAR1 = -0; VAR2= -Infinity; VAR2 %= VAR1", 
              Number.NaN,     
              eval("VAR1 = -0; VAR2 = Number.NEGATIVE_INFINITY; VAR2 %= VAR1; VAR2") );

new TestCase( SECTION,   
              "VAR1 = 0; VAR2= -Infinity; VAR2 %= VAR1",  
              Number.NaN,     
              eval("VAR1 = 0; VAR2 = Number.NEGATIVE_INFINITY; VAR2 %= VAR1; VAR2") );

new TestCase( SECTION,   
              "VAR1 = 1; VAR2= Infinity; VAR2 %= VAR1",   
              Number.NaN,     
              eval("VAR1 = 1; VAR2 = Number.POSITIVE_INFINITY; VAR2 %= VAR1; VAR2") );

new TestCase( SECTION,   
              "VAR1 = -1; VAR2= Infinity; VAR2 %= VAR1",  
              Number.NaN,     
              eval("VAR1 = -1; VAR2 = Number.POSITIVE_INFINITY; VAR2 %= VAR1; VAR2") );

new TestCase( SECTION,   
              "VAR1 = -1; VAR2= -Infinity; VAR2 %= VAR1", 
              Number.NaN,     
              eval("VAR1 = -1; VAR2 = Number.NEGATIVE_INFINITY; VAR2 %= VAR1; VAR2") );

new TestCase( SECTION,   
              "VAR1 = 1; VAR2= -Infinity; VAR2 %= VAR1",  
              Number.NaN,     
              eval("VAR1 = 1; VAR2 = Number.NEGATIVE_INFINITY; VAR2 %= VAR1; VAR2") );

new TestCase( SECTION,   
              "VAR1 = 0; VAR2= 0; VAR1 %= VAR2",   
              Number.NaN,     
              eval("VAR1 = 0; VAR2 = 0; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = 0; VAR2= -0; VAR1 %= VAR2",  
              Number.NaN,     
              eval("VAR1 = 0; VAR2 = -0; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = -0; VAR2= 0; VAR1 %= VAR2",  
              Number.NaN,     
              eval("VAR1 = -0; VAR2 = 0; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = -0; VAR2= -0; VAR1 %= VAR2", 
              Number.NaN,     
              eval("VAR1 = -0; VAR2 = -0; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = 1; VAR2= 0; VAR1 %= VAR2",   
              Number.NaN,     
              eval("VAR1 = 1; VAR2 = 0; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = 1; VAR2= -0; VAR1 %= VAR2",  
              Number.NaN,     
              eval("VAR1 = 1; VAR2 = -0; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = -1; VAR2= 0; VAR1 %= VAR2",  
              Number.NaN,     
              eval("VAR1 = -1; VAR2 = 0; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = -1; VAR2= -0; VAR1 %= VAR2", 
              Number.NaN,     
              eval("VAR1 = -1; VAR2 = -0; VAR1 %= VAR2; VAR1") );



new TestCase( SECTION,   
              "VAR1 = 0; VAR2= Infinity; VAR1 %= VAR2;VAR1",   
              0,     
              eval("VAR1 = 0; VAR2 = Number.POSITIVE_INFINITY; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = -0; VAR2= Infinity; VAR1 %= VAR2;VAR1",  
              -0,    
              eval("VAR1 = -0; VAR2 = Number.POSITIVE_INFINITY; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = -0; VAR2= -Infinity; VAR1 %= VAR2;VAR1", 
              -0,    
              eval("VAR1 = -0; VAR2 = Number.NEGATIVE_INFINITY; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = 0; VAR2= -Infinity; VAR1 %= VAR2;VAR1",  
              0,     
              eval("VAR1 = 0; VAR2 = Number.NEGATIVE_INFINITY; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = 1; VAR2= Infinity; VAR1 %= VAR2;VAR1",   
              1,     
              eval("VAR1 = 1; VAR2 = Number.POSITIVE_INFINITY; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = -1; VAR2= Infinity; VAR1 %= VAR2;VAR1",  
              -1,    
              eval("VAR1 = -1; VAR2 = Number.POSITIVE_INFINITY; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = -1; VAR2= -Infinity; VAR1 %= VAR2;VAR1", 
              -1,    
              eval("VAR1 = -1; VAR2 = Number.NEGATIVE_INFINITY; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = 1; VAR2= -Infinity; VAR1 %= VAR2;VAR1",  
              1,     
              eval("VAR1 = 1; VAR2 = Number.NEGATIVE_INFINITY; VAR1 %= VAR2; VAR1") );



new TestCase( SECTION,   
              "VAR1 = 0; VAR2= 1; VAR1 %= VAR2; VAR1",   
              0,   
              eval("VAR1 = 0; VAR2 = 1; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = -0; VAR2= 1; VAR1 %= VAR2; VAR1",  
              -0,  
              eval("VAR1 = -0; VAR2 = 1; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = -0; VAR2= -1; VAR1 %= VAR2; VAR1", 
              -0,  
              eval("VAR1 = -0; VAR2 = -1; VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = 0; VAR2= -1; VAR1 %= VAR2; VAR1",  
              0,   
              eval("VAR1 = 0; VAR2 = -1; VAR1 %= VAR2; VAR1") );


new TestCase( SECTION,   
              "VAR1 = 1000; VAR2 = '10', VAR1 %= VAR2; VAR1",
              0,      
              eval("VAR1 = 1000; VAR2 = '10', VAR1 %= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = '1000'; VAR2 = 10, VAR1 %= VAR2; VAR1",
              0,      
              eval("VAR1 = '1000'; VAR2 = 10, VAR1 %= VAR2; VAR1") );




















test();

