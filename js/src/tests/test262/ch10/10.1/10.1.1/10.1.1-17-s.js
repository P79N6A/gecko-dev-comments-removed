











function testcase() {
        "use strict";
        try {
            var obj = {};
            Object.defineProperty(obj, "accProperty", {
                get: function () {
                    eval("public = 1;");
                    return 11;
                }
            });

            var temp = obj.accProperty === 11;
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
