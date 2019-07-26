










function testcase() {
        var obj = {};
        try {
            eval("function obj.tt() {};");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
