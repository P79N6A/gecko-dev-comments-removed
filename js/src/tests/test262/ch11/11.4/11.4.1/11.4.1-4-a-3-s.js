











function testcase() {
        "use strict";
        var obj = {};
        Object.defineProperty(obj, "prop", {
            value: "abc",
            configurable: true
        });

        delete obj.prop;
        return !obj.hasOwnProperty("prop");
    }
runTestCase(testcase);
