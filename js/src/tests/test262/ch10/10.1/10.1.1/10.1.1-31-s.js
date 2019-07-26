











function testcase() {
        var funObj = new Function("a", "eval('public = 1;'); 'use strict'; anotherVariable = 2;");
        funObj();
        return public === 1 && anotherVariable === 2;
    }
runTestCase(testcase);
