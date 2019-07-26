











function testcase() {
        "use strict";
        var obj = {};
        Object.defineProperty(obj, "prop", {
            get: function () {
                return "abc"; 
            },
            configurable: false
        });

        try {
            delete obj.prop;
            return false;
        } catch (e) {
            return e instanceof TypeError && obj.prop === "abc";
        }
    }
runTestCase(testcase);
