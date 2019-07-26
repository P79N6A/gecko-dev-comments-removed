











function testcase() {
        var obj = {};
        Object.defineProperty(obj, "accProperty", {
            get: function () {
                eval("public = 1;");
                "use strict";
                return 11;
            }
        });
        return obj.accProperty === 11 && public === 1;
    }
runTestCase(testcase);
