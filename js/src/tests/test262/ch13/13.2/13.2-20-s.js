












function testcase() {
            var foo = Function("'use strict'; for (var tempIndex in this) {if (tempIndex===\"arguments\") {return false;}}; return true;");
            return foo();
}
runTestCase(testcase);