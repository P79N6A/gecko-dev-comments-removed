










function testcase() {
        try {
            throw function () {
                this._12_14_14_foo = "test";
            };
            return false;
        } catch (e) {
            e();
            return fnGlobalObject()._12_14_14_foo === "test";
        }
        finally {
           delete fnGlobalObject()._12_14_14_foo;
        }
    }
runTestCase(testcase);
