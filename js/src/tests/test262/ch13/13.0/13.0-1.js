










function testcase() {
        try {
            eval("function x, y() {}");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
