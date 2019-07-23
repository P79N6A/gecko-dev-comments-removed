





































gTestfile = '15.6.4.3-2.js';















var SECTION = "15.6.4.3-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Boolean.prototype.valueOf()";

writeHeaderToLog( SECTION + " "+ TITLE);


new TestCase( SECTION, "valof=Boolean.prototype.valueOf; x=new Boolean(); x.valueOf=valof;x.valueOf()", false, eval("valof=Boolean.prototype.valueOf; x=new Boolean(); x.valueOf=valof;x.valueOf()") );
   
new TestCase( SECTION, "valof=Boolean.prototype.valueOf; x=new Boolean(true); x.valueOf=valof;x.valueOf()", true, eval("valof=Boolean.prototype.valueOf; x=new Boolean(true); x.valueOf=valof;x.valueOf()") );

test();
