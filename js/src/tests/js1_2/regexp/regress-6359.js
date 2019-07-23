




































gTestfile = 'regress-6359.js';








var SECTION = "js1_2";       
var VERSION = "ECMA_2"; 
var TITLE   = "Regression test for bugzilla # 6359";       
var BUGNUMBER = "http://bugzilla.mozilla.org/show_bug.cgi?id=6359";     

startTest();               



















AddTestCase( '/(a*)b\1+/("baaac").length',
	     2,
	     /(a*)b\1+/("baaac").length );

AddTestCase( '/(a*)b\1+/("baaac")[0]',
	     "b",
	     /(a*)b\1+/("baaac")[0]);

AddTestCase( '/(a*)b\1+/("baaac")[1]',
	     "",
	     /(a*)b\1+/("baaac")[1]);


test();       

