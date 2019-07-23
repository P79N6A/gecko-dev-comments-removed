





































gTestfile = '10.1.3.js';









var SECTION = "10.1.3";
var VERSION = "ECMA_1";
var TITLE   = "Variable instantiation";
var BUGNUMBER = "20256";
startTest();

writeHeaderToLog( SECTION + " "+ TITLE);

   

    
new TestCase(SECTION,
	     "function t() { return \"first\" };" +
	     "function t() { return \"second\" };t() ",
	     "second",
	     eval("function t() { return \"first\" };" +
		  "function t() { return \"second\" };t()"));

   
new TestCase(SECTION,
	     "var t; function t(){}; typeof(t)",
	     "function",
	     eval("var t; function t(){}; typeof(t)"));



    
new TestCase(SECTION,
	     "function t1(a,b) { return b; }; t1( 4 );",
	     void 0,
	     eval("function t1(a,b) { return b; }; t1( 4 );") );
   
new TestCase(SECTION,
	     "function t1(a,b) { return a; }; t1(4);",
	     4,
	     eval("function t1(a,b) { return a; }; t1(4)"));
    
new TestCase(SECTION,
	     "function t1(a,b) { return a; }; t1();",
	     void 0,
	     eval("function t1(a,b) { return a; }; t1()"));
   
new TestCase(SECTION,
	     "function t1(a,b) { return a; }; t1(1,2,4);",
	     1,
	     eval("function t1(a,b) { return a; }; t1(1,2,4)"));












   
new TestCase(SECTION,
	     "function t1(a,b) { return a; }; t1( false, true );",
	     false,
	     eval("function t1(a,b) { return a; }; t1( false, true );"));
   
new TestCase(SECTION,
	     "function t1(a,b) { return b; }; t1( false, true );",
	     true,
	     eval("function t1(a,b) { return b; }; t1( false, true );"));
   
new TestCase(SECTION,
	     "function t1(a,b) { return a+b; }; t1( 4, 2 );",
	     6,
	     eval("function t1(a,b) { return a+b; }; t1( 4, 2 );"));
   
new TestCase(SECTION,
	     "function t1(a,b) { return a+b; }; t1( 4 );",
	     Number.NaN,
	     eval("function t1(a,b) { return a+b; }; t1( 4 );"));


   
new TestCase(SECTION,
	     "function t() { return 'function' };" +
	     "var t = 'variable'; typeof(t)",
	     "string",
	     eval("function t() { return 'function' };" +
		  "var t = 'variable'; typeof(t)"));


   
new TestCase(SECTION,
	     "function t1(a,b) { var a = b; return a; } t1(1,3);",
	     3,
	     eval("function t1(a, b){ var a = b; return a;}; t1(1,3)"));
   
new TestCase(SECTION,
	     "function t2(a,b) { this.a = b;  } x  = new t2(1,3); x.a",
	     3,
	     eval("function t2(a,b) { this.a = b; };" +
		  "x = new t2(1,3); x.a"));
   
new TestCase(SECTION,
	     "function t2(a,b) { this.a = a;  } x  = new t2(1,3); x.a",
	     1,
	     eval("function t2(a,b) { this.a = a; };" +
		  "x = new t2(1,3); x.a"));
   
new TestCase(SECTION,
	     "function t2(a,b) { this.a = b; this.b = a; } " +
	     "x = new t2(1,3);x.a;",
	     3,
	     eval("function t2(a,b) { this.a = b; this.b = a; };" +
		  "x = new t2(1,3);x.a;"));
   
new TestCase(SECTION,
	     "function t2(a,b) { this.a = b; this.b = a; }" +
	     "x = new t2(1,3);x.b;",
	     1,
	     eval("function t2(a,b) { this.a = b; this.b = a; };" +
		  "x = new t2(1,3);x.b;") );

test();
