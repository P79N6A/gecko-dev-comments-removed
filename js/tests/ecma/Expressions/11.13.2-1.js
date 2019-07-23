





































gTestfile = '11.13.2-1.js';

























var SECTION = "11.13.2-1";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Compound Assignment: *=");




new TestCase( SECTION,   
              "VAR1 = NaN; VAR2=1; VAR1 *= VAR2",      
              Number.NaN,
              eval("VAR1 = Number.NaN; VAR2=1; VAR1 *= VAR2") );

new TestCase( SECTION,   
              "VAR1 = NaN; VAR2=1; VAR1 *= VAR2; VAR1",
              Number.NaN,
              eval("VAR1 = Number.NaN; VAR2=1; VAR1 *= VAR2; VAR1") );


new TestCase( SECTION,   
              "VAR1 = 0; VAR2=1; VAR1 *= VAR2",        
              0,         
              eval("VAR1 = 0; VAR2=1; VAR1 *= VAR2") );

new TestCase( SECTION,   
              "VAR1 = 0; VAR2=1; VAR1 *= VAR2;VAR1",   
              0,         
              eval("VAR1 = 0; VAR2=1; VAR1 *= VAR2;VAR1") );

new TestCase( SECTION,   
              "VAR1 = 0xFF; VAR2 = 0xA, VAR1 *= VAR2",
              2550,     
              eval("VAR1 = 0XFF; VAR2 = 0XA, VAR1 *= VAR2") );



new TestCase( SECTION,   
              "VAR1 = 0; VAR2= Infinity; VAR1 *= VAR2",   
              Number.NaN,     
              eval("VAR1 = 0; VAR2 = Number.POSITIVE_INFINITY; VAR1 *= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = -0; VAR2= Infinity; VAR1 *= VAR2",  
              Number.NaN,     
              eval("VAR1 = -0; VAR2 = Number.POSITIVE_INFINITY; VAR1 *= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = -0; VAR2= -Infinity; VAR1 *= VAR2", 
              Number.NaN,     
              eval("VAR1 = -0; VAR2 = Number.NEGATIVE_INFINITY; VAR1 *= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = 0; VAR2= -Infinity; VAR1 *= VAR2",  
              Number.NaN,     
              eval("VAR1 = 0; VAR2 = Number.NEGATIVE_INFINITY; VAR1 *= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = 0; VAR2= Infinity; VAR2 *= VAR1",   
              Number.NaN,     
              eval("VAR1 = 0; VAR2 = Number.POSITIVE_INFINITY; VAR2 *= VAR1; VAR2") );

new TestCase( SECTION,   
              "VAR1 = -0; VAR2= Infinity; VAR2 *= VAR1",  
              Number.NaN,     
              eval("VAR1 = -0; VAR2 = Number.POSITIVE_INFINITY; VAR2 *= VAR1; VAR2") );

new TestCase( SECTION,   
              "VAR1 = -0; VAR2= -Infinity; VAR2 *= VAR1", 
              Number.NaN,     
              eval("VAR1 = -0; VAR2 = Number.NEGATIVE_INFINITY; VAR2 *= VAR1; VAR2") );

new TestCase( SECTION,   
              "VAR1 = 0; VAR2= -Infinity; VAR2 *= VAR1",  
              Number.NaN,     
              eval("VAR1 = 0; VAR2 = Number.NEGATIVE_INFINITY; VAR2 *= VAR1; VAR2") );

new TestCase( SECTION,   
              "VAR1 = Infinity; VAR2= Infinity; VAR1 *= VAR2",  
              Number.POSITIVE_INFINITY,     
              eval("VAR1 = Number.POSITIVE_INFINITY; VAR2 = Number.POSITIVE_INFINITY; VAR1 *= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = Infinity; VAR2= -Infinity; VAR1 *= VAR2", 
              Number.NEGATIVE_INFINITY,     
              eval("VAR1 = Number.POSITIVE_INFINITY; VAR2 = Number.NEGATIVE_INFINITY; VAR1 *= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 =-Infinity; VAR2= Infinity; VAR1 *= VAR2",  
              Number.NEGATIVE_INFINITY,     
              eval("VAR1 = Number.NEGATIVE_INFINITY; VAR2 = Number.POSITIVE_INFINITY; VAR1 *= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 =-Infinity; VAR2=-Infinity; VAR1 *= VAR2",  
              Number.POSITIVE_INFINITY,     
              eval("VAR1 = Number.NEGATIVE_INFINITY; VAR2 = Number.NEGATIVE_INFINITY; VAR1 *= VAR2; VAR1") );


new TestCase( SECTION,   
              "VAR1 = 10; VAR2 = '255', VAR1 *= VAR2",
              2550,      
              eval("VAR1 = 10; VAR2 = '255', VAR1 *= VAR2") );

new TestCase( SECTION,   
              "VAR1 = '255'; VAR2 = 10, VAR1 *= VAR2",
              2550,      
              eval("VAR1 = '255'; VAR2 = 10, VAR1 *= VAR2") );

new TestCase( SECTION,   
              "VAR1 = 10; VAR2 = '0XFF', VAR1 *= VAR2",
              2550,      
              eval("VAR1 = 10; VAR2 = '0XFF', VAR1 *= VAR2") );

new TestCase( SECTION,   
              "VAR1 = '0xFF'; VAR2 = 0xA, VAR1 *= VAR2",
              2550,     
              eval("VAR1 = '0XFF'; VAR2 = 0XA, VAR1 *= VAR2") );

new TestCase( SECTION,   
              "VAR1 = '10'; VAR2 = '255', VAR1 *= VAR2",
              2550,     
              eval("VAR1 = '10'; VAR2 = '255', VAR1 *= VAR2") );

new TestCase( SECTION,   
              "VAR1 = '10'; VAR2 = '0XFF', VAR1 *= VAR2",
              2550,    
              eval("VAR1 = '10'; VAR2 = '0XFF', VAR1 *= VAR2") );

new TestCase( SECTION,   
              "VAR1 = '0xFF'; VAR2 = 0xA, VAR1 *= VAR2",
              2550,     
              eval("VAR1 = '0XFF'; VAR2 = 0XA, VAR1 *= VAR2") );


new TestCase( SECTION,   
              "VAR1 = true; VAR2 = false; VAR1 *= VAR2",   
              0,     
              eval("VAR1 = true; VAR2 = false; VAR1 *= VAR2") );

new TestCase( SECTION,   
              "VAR1 = true; VAR2 = true; VAR1 *= VAR2",   
              1,     
              eval("VAR1 = true; VAR2 = true; VAR1 *= VAR2") );


new TestCase( SECTION,   
              "VAR1 = new Boolean(true); VAR2 = 10; VAR1 *= VAR2;VAR1",   
              10,     
              eval("VAR1 = new Boolean(true); VAR2 = 10; VAR1 *= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = new Number(11); VAR2 = 10; VAR1 *= VAR2; VAR1",   
              110,     
              eval("VAR1 = new Number(11); VAR2 = 10; VAR1 *= VAR2; VAR1") );

new TestCase( SECTION,   
              "VAR1 = new Number(11); VAR2 = new Number(10); VAR1 *= VAR2",   
              110,     
              eval("VAR1 = new Number(11); VAR2 = new Number(10); VAR1 *= VAR2") );

new TestCase( SECTION,   
              "VAR1 = new String('15'); VAR2 = new String('0xF'); VAR1 *= VAR2",   
              225,     
              eval("VAR1 = String('15'); VAR2 = new String('0xF'); VAR1 *= VAR2") );

test();

