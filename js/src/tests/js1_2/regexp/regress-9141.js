




































gTestfile = 'regress-9141.js';


























var SECTION = "js1_2";       
var VERSION = "ECMA_2"; 
var TITLE   = "Regression test for bugzilla # 9141";       
var BUGNUMBER = "http://bugzilla.mozilla.org/show_bug.cgi?id=9141";     

startTest();               



















var s = "x";
for (var i = 0; i != 13; i++) s += s;
var a = /(?:xx|x)*/(s);
var b = /(xx|x)*/(s);

AddTestCase( "var s = 'x'; for (var i = 0; i != 13; i++) s += s; " +
	     "a = /(?:xx|x)*/(s); a.length",
	     1,
	     a.length );

AddTestCase( "var b = /(xx|x)*/(s); b.length",
	     2,
	     b.length );

test();       

