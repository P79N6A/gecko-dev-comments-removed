











function testcase() {
        try {
            eval("'use strict'; var public = 1; var anotherVariableNotReserveWord = 2;");

            return false;
        } catch (e) {
            return e instanceof SyntaxError && typeof public === "undefined" &&
                typeof anotherVariableNotReserveWord === "undefined";
        }
    }
runTestCase(testcase);
