










function testcase() {
        var obj = {};
        obj.test = function () {
            this._12_14_15_foo = "test";
        };
        try {
            throw obj.test;
            return false;
        } catch (e) {
            e();
            return  fnGlobalObject()._12_14_15_foo === "test";
        }
        finally {
            delete fnGlobalObject()._12_14_15_foo;
        }
    }
runTestCase(testcase);
