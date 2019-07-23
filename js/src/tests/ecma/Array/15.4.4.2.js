





































gTestfile = '15.4.4.2.js';













var SECTION = "15.4.4.2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Array.prototype.toString";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION, 
	      "Array.prototype.toString.length", 
	      0, 
	      Array.prototype.toString.length );

new TestCase( SECTION, 
	      "(new Array()).toString()",    
	      "",    
	      (new Array()).toString() );

new TestCase( SECTION, 
	      "(new Array(2)).toString()",   
	      ",",   
	      (new Array(2)).toString() );

new TestCase( SECTION, 
	      "(new Array(0,1)).toString()", 
	      "0,1", 
	      (new Array(0,1)).toString() );

new TestCase( SECTION, 
	      "(new Array( Number.NaN, Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY)).toString()", 
	      "NaN,Infinity,-Infinity",  
	      (new Array( Number.NaN, Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY)).toString() );

new TestCase( SECTION, 
	      "(new Array( Boolean(1), Boolean(0))).toString()",  
	      "true,false",  
	      (new Array(Boolean(1),Boolean(0))).toString() );

new TestCase( SECTION, 
	      "(new Array(void 0,null)).toString()",   
	      ",",   
	      (new Array(void 0,null)).toString() );

var EXPECT_STRING = "";
var MYARR = new Array();

for ( var i = -50; i < 50; i+= 0.25 ) {
  MYARR[MYARR.length] = i;
  EXPECT_STRING += i +",";
}

EXPECT_STRING = EXPECT_STRING.substring( 0, EXPECT_STRING.length -1 );

new TestCase( SECTION,
	      "MYARR.toString()", 
	      EXPECT_STRING, 
	      MYARR.toString() );

test();
