





































gTestfile = '11.12-2-n.js';


















var SECTION = "11.12-2-n";
var VERSION = "ECMA_1";
startTest();
writeHeaderToLog( SECTION + " Conditional operator ( ? : )");



DESCRIPTION = "var MYVAR =  true ? 'EXPR1', 'EXPR2' : 'EXPR3'; MYVAR";
EXPECTED = "error";

new TestCase( SECTION,
              "var MYVAR =  true ? 'EXPR1', 'EXPR2' : 'EXPR3'; MYVAR",
              "error",
              eval("var MYVAR =  true ? 'EXPR1', 'EXPR2' : 'EXPR3'; MYVAR") );

test();

