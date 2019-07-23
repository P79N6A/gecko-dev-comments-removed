





































gTestfile = '15.1.2.2-1.js';






































































var SECTION = "15.1.2.2-1";
var VERSION = "ECMA_1";
var TITLE   = "parseInt(string, radix)";
var BUGNUMBER = "none";

startTest();

writeHeaderToLog( SECTION + " "+ TITLE);

var HEX_STRING = "0x0";
var HEX_VALUE = 0;

new TestCase( SECTION, 
	      "parseInt.length",     
	      2,     
	      parseInt.length );

new TestCase( SECTION, 
	      "parseInt.length = 0; parseInt.length",    
	      2,     
	      eval("parseInt.length = 0; parseInt.length") );

new TestCase( SECTION, 
	      "var PROPS=''; for ( var p in parseInt ) { PROPS += p; }; PROPS",   "",
	      eval("var PROPS=''; for ( var p in parseInt ) { PROPS += p; }; PROPS") );

new TestCase( SECTION, 
	      "delete parseInt.length",  
	      false, 
	      delete parseInt.length );

new TestCase( SECTION, 
	      "delete parseInt.length; parseInt.length", 
	      2, 
	      eval("delete parseInt.length; parseInt.length") );

new TestCase( SECTION, 
	      "parseInt.length = null; parseInt.length", 
	      2, 
	      eval("parseInt.length = null; parseInt.length") );

new TestCase( SECTION, 
	      "parseInt()",      
	      NaN,   
	      parseInt() );

new TestCase( SECTION, 
	      "parseInt('')",    
	      NaN,   
	      parseInt("") );

new TestCase( SECTION, 
	      "parseInt('','')", 
	      NaN,   
	      parseInt("","") );

new TestCase( SECTION,
	      "parseInt(\"     0xabcdef     ",
	      11259375,
	      parseInt( "      0xabcdef     " ));

new TestCase( SECTION,
	      "parseInt(\"     0XABCDEF     ",
	      11259375,
	      parseInt( "      0XABCDEF     " ) );

new TestCase( SECTION,
	      "parseInt( 0xabcdef )",
	      11259375,
	      parseInt( "0xabcdef") );

new TestCase( SECTION,
	      "parseInt( 0XABCDEF )",
	      11259375,
	      parseInt( "0XABCDEF") );

for ( HEX_STRING = "0x0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
  new TestCase( SECTION, "parseInt("+HEX_STRING+")",    HEX_VALUE,  parseInt(HEX_STRING) );
  HEX_VALUE += Math.pow(16,POWER)*15;
}
for ( HEX_STRING = "0X0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
  new TestCase( SECTION, "parseInt("+HEX_STRING+")",    HEX_VALUE,  parseInt(HEX_STRING) );
  HEX_VALUE += Math.pow(16,POWER)*15;
}
for ( HEX_STRING = "0x0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
  new TestCase( SECTION, "parseInt("+HEX_STRING+",16)",    HEX_VALUE,  parseInt(HEX_STRING,16) );
  HEX_VALUE += Math.pow(16,POWER)*15;
}
for ( HEX_STRING = "0x0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
  new TestCase( SECTION, "parseInt("+HEX_STRING+",16)",    HEX_VALUE,  parseInt(HEX_STRING,16) );
  HEX_VALUE += Math.pow(16,POWER)*15;
}
for ( HEX_STRING = "0x0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
  new TestCase( SECTION, "parseInt("+HEX_STRING+",null)",    HEX_VALUE,  parseInt(HEX_STRING,null) );
  HEX_VALUE += Math.pow(16,POWER)*15;
}
for ( HEX_STRING = "0x0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
  new TestCase( SECTION, "parseInt("+HEX_STRING+", void 0)",    HEX_VALUE,  parseInt(HEX_STRING, void 0) );
  HEX_VALUE += Math.pow(16,POWER)*15;
}



for ( var space = " ", HEX_STRING = "0x0", HEX_VALUE = 0, POWER = 0;
      POWER < 15;
      POWER++, HEX_STRING = HEX_STRING +"f", space += " ")
{
  new TestCase( SECTION, "parseInt("+space+HEX_STRING+space+", void 0)",    HEX_VALUE,  parseInt(space+HEX_STRING+space, void 0) );
  HEX_VALUE += Math.pow(16,POWER)*15;
}


for ( HEX_STRING = "-0x0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
  new TestCase( SECTION, "parseInt("+HEX_STRING+")",    HEX_VALUE,  parseInt(HEX_STRING) );
  HEX_VALUE -= Math.pow(16,POWER)*15;
}



for ( HEX_STRING = "0x0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
  new TestCase( SECTION, "parseInt("+HEX_STRING+"g,16)",    HEX_VALUE,  parseInt(HEX_STRING+"g",16) );
  HEX_VALUE += Math.pow(16,POWER)*15;
}
for ( HEX_STRING = "0x0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
  new TestCase( SECTION, "parseInt("+HEX_STRING+"g,16)",    HEX_VALUE,  parseInt(HEX_STRING+"G",16) );
  HEX_VALUE += Math.pow(16,POWER)*15;
}

for ( HEX_STRING = "-0x0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
  new TestCase( SECTION, "parseInt("+HEX_STRING+")",    HEX_VALUE,  parseInt(HEX_STRING) );
  HEX_VALUE -= Math.pow(16,POWER)*15;
}
for ( HEX_STRING = "-0X0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
  new TestCase( SECTION, "parseInt("+HEX_STRING+")",    HEX_VALUE,  parseInt(HEX_STRING) );
  HEX_VALUE -= Math.pow(16,POWER)*15;
}
for ( HEX_STRING = "-0x0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
  new TestCase( SECTION, "parseInt("+HEX_STRING+",16)",    HEX_VALUE,  parseInt(HEX_STRING,16) );
  HEX_VALUE -= Math.pow(16,POWER)*15;
}
for ( HEX_STRING = "-0x0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
  new TestCase( SECTION, "parseInt("+HEX_STRING+",16)",    HEX_VALUE,  parseInt(HEX_STRING,16) );
  HEX_VALUE -= Math.pow(16,POWER)*15;
}




var OCT_STRING = "0";
var OCT_VALUE = 0;

for ( OCT_STRING = "0", OCT_VALUE = 0, POWER = 0; POWER < 15; POWER++, OCT_STRING = OCT_STRING +"7" ) {
  new TestCase( SECTION, "parseInt("+OCT_STRING+")",    OCT_VALUE,  parseInt(OCT_STRING) );
  OCT_VALUE += Math.pow(8,POWER)*7;
}

for ( OCT_STRING = "-0", OCT_VALUE = 0, POWER = 0; POWER < 15; POWER++, OCT_STRING = OCT_STRING +"7" ) {
  new TestCase( SECTION, "parseInt("+OCT_STRING+")",    OCT_VALUE,  parseInt(OCT_STRING) );
  OCT_VALUE -= Math.pow(8,POWER)*7;
}



for ( OCT_STRING = "0", OCT_VALUE = 0, POWER = 0; POWER < 15; POWER++, OCT_STRING = OCT_STRING +"7" ) {
  new TestCase( SECTION, "parseInt("+OCT_STRING+",8)",    OCT_VALUE,  parseInt(OCT_STRING,8) );
  OCT_VALUE += Math.pow(8,POWER)*7;
}
for ( OCT_STRING = "-0", OCT_VALUE = 0, POWER = 0; POWER < 15; POWER++, OCT_STRING = OCT_STRING +"7" ) {
  new TestCase( SECTION, "parseInt("+OCT_STRING+",010)",    OCT_VALUE,  parseInt(OCT_STRING,010) );
  OCT_VALUE -= Math.pow(8,POWER)*7;
}



for ( OCT_STRING = "0", OCT_VALUE = 0, POWER = 0; POWER < 15; POWER++, OCT_STRING = OCT_STRING +"7" ) {
  new TestCase( SECTION, "parseInt("+OCT_STRING+"8,8)",    OCT_VALUE,  parseInt(OCT_STRING+"8",8) );
  OCT_VALUE += Math.pow(8,POWER)*7;
}
for ( OCT_STRING = "-0", OCT_VALUE = 0, POWER = 0; POWER < 15; POWER++, OCT_STRING = OCT_STRING +"7" ) {
  new TestCase( SECTION, "parseInt("+OCT_STRING+"8,010)",    OCT_VALUE,  parseInt(OCT_STRING+"8",010) );
  OCT_VALUE -= Math.pow(8,POWER)*7;
}

new TestCase( SECTION,
	      "parseInt( '0x' )",             
	      NaN,       
	      parseInt("0x") );

new TestCase( SECTION,
	      "parseInt( '0X' )",             
	      NaN,       
	      parseInt("0X") );

new TestCase( SECTION,
	      "parseInt( '11111111112222222222' )",   
	      11111111112222222222,  
	      parseInt("11111111112222222222") );

new TestCase( SECTION,
	      "parseInt( '111111111122222222223' )",   
	      111111111122222222220,  
	      parseInt("111111111122222222223") );

new TestCase( SECTION,
	      "parseInt( '11111111112222222222',10 )",   
	      11111111112222222222,  
	      parseInt("11111111112222222222",10) );

new TestCase( SECTION,
	      "parseInt( '111111111122222222223',10 )",   
	      111111111122222222220,  
	      parseInt("111111111122222222223",10) );

new TestCase( SECTION,
	      "parseInt( '01234567890', -1 )", 
	      Number.NaN,   
	      parseInt("01234567890",-1) );

new TestCase( SECTION,
	      "parseInt( '01234567890', 0 )", 
	      Number.NaN,    
	      parseInt("01234567890",1) );

new TestCase( SECTION,
	      "parseInt( '01234567890', 1 )", 
	      Number.NaN,    
	      parseInt("01234567890",1) );

new TestCase( SECTION,
	      "parseInt( '01234567890', 2 )", 
	      1,             
	      parseInt("01234567890",2) );

new TestCase( SECTION,
	      "parseInt( '01234567890', 3 )", 
	      5,             
	      parseInt("01234567890",3) );

new TestCase( SECTION,
	      "parseInt( '01234567890', 4 )", 
	      27,            
	      parseInt("01234567890",4) );

new TestCase( SECTION,
	      "parseInt( '01234567890', 5 )", 
	      194,           
	      parseInt("01234567890",5) );

new TestCase( SECTION,
	      "parseInt( '01234567890', 6 )", 
	      1865,          
	      parseInt("01234567890",6) );

new TestCase( SECTION,
	      "parseInt( '01234567890', 7 )", 
	      22875,         
	      parseInt("01234567890",7) );

new TestCase( SECTION,
	      "parseInt( '01234567890', 8 )", 
	      342391,        
	      parseInt("01234567890",8) );

new TestCase( SECTION,
	      "parseInt( '01234567890', 9 )", 
	      6053444,       
	      parseInt("01234567890",9) );

new TestCase( SECTION,
	      "parseInt( '01234567890', 10 )",
	      1234567890,    
	      parseInt("01234567890",10) );



new TestCase( SECTION,
	      "parseInt( '1234567890', '0xa')",
	      1234567890,
	      parseInt("1234567890","0xa") );

new TestCase( SECTION,
	      "parseInt( '012345', 11 )",     
	      17715,         
	      parseInt("012345",11) );

new TestCase( SECTION,
	      "parseInt( '012345', 35 )",     
	      1590195,       
	      parseInt("012345",35) );

new TestCase( SECTION,
	      "parseInt( '012345', 36 )",     
	      1776965,       
	      parseInt("012345",36) );

new TestCase( SECTION,
	      "parseInt( '012345', 37 )",     
	      Number.NaN,    
	      parseInt("012345",37) );

test();
