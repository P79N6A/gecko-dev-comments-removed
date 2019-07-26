













function testcase() {

    var indirectEval = eval;
	
    try {
	    indirectEval("'use strict'; var arguments;");
        return false;
	}
    catch (e) {
        return (e instanceof SyntaxError);
	}
}
runTestCase(testcase);