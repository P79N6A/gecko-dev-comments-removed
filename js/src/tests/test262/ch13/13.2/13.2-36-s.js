












function testcase() {
        var funcExpr = function () { "use strict";};
        return Object.getOwnPropertyDescriptor(funcExpr, 
                                                "arguments") === undefined;
}
runTestCase(testcase);
