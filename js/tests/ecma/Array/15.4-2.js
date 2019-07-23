





































gTestfile = '15.4-2.js';




















var SECTION = "15.4-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Array Objects";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
              "var arr=new Array();  arr[Math.pow(2,16)] = 'hi'; arr.length",     
              Math.pow(2,16)+1,  
              eval("var arr=new Array();  arr[Math.pow(2,16)] = 'hi'; arr.length") );

new TestCase( SECTION,
              "var arr=new Array();  arr[Math.pow(2,30)-2] = 'hi'; arr.length",   
              Math.pow(2,30)-1,  
              eval("var arr=new Array();  arr[Math.pow(2,30)-2] = 'hi'; arr.length") );

new TestCase( SECTION,
              "var arr=new Array();  arr[Math.pow(2,30)-1] = 'hi'; arr.length",   
              Math.pow(2,30),    
              eval("var arr=new Array();  arr[Math.pow(2,30)-1] = 'hi'; arr.length") );

new TestCase( SECTION,
              "var arr=new Array();  arr[Math.pow(2,30)] = 'hi'; arr.length",     
              Math.pow(2,30)+1,  
              eval("var arr=new Array();  arr[Math.pow(2,30)] = 'hi'; arr.length") );


new TestCase( SECTION,
              "var arr=new Array();  arr[Math.pow(2,31)-2] = 'hi'; arr.length",   
              Math.pow(2,31)-1,  
              eval("var arr=new Array();  arr[Math.pow(2,31)-2] = 'hi'; arr.length") );

new TestCase( SECTION,
              "var arr=new Array();  arr[Math.pow(2,31)-1] = 'hi'; arr.length",   
              Math.pow(2,31),    
              eval("var arr=new Array();  arr[Math.pow(2,31)-1] = 'hi'; arr.length") );

new TestCase( SECTION,
              "var arr=new Array();  arr[Math.pow(2,31)] = 'hi'; arr.length",     
              Math.pow(2,31)+1,  
              eval("var arr=new Array();  arr[Math.pow(2,31)] = 'hi'; arr.length") );

new TestCase( SECTION,
              "var arr = new Array(0,1,2,3,4,5); arr.length = 2; String(arr)",    
              "0,1",             
              eval("var arr = new Array(0,1,2,3,4,5); arr.length = 2; String(arr)") );

new TestCase( SECTION,
              "var arr = new Array(0,1); arr.length = 3; String(arr)",            
              "0,1,",            
              eval("var arr = new Array(0,1); arr.length = 3; String(arr)") );

test();

